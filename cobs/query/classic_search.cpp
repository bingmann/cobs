/*******************************************************************************
 * cobs/query/classic_search.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/classic_search.hpp>
#include <cobs/util/error_handling.hpp>

#include <algorithm>
#include <cobs/kmer.hpp>
#include <cobs/settings.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/misc.hpp>
#include <cobs/util/parallel_for.hpp>
#include <cobs/util/query.hpp>
#include <cobs/util/timer.hpp>
#include <numeric>
#include <string>
#include <vector>

#include <tlx/logger.hpp>
#include <tlx/math/div_ceil.hpp>
#include <tlx/math/round_up.hpp>
#include <tlx/simple_vector.hpp>

#include <xxhash.h>

#if __SSE2__
#include <immintrin.h>
#endif

namespace cobs {

void ClassicSearch::create_hashes(std::vector<uint64_t>& hashes,
                                  const std::string& query) {

    uint32_t term_size = index_file_.term_size();
    size_t num_hashes = index_file_.num_hashes();
    uint8_t canonicalize = index_file_.canonicalize();

    size_t num_terms = query.size() - term_size + 1;
    hashes.resize(num_hashes * num_terms);

    const char* query_8 = query.data();

    if (canonicalize == 0) {
        for (size_t i = 0; i < num_terms; i++) {
            for (size_t j = 0; j < num_hashes; j++) {
                hashes[i * num_hashes + j] = XXH64(query_8 + i, term_size, j);
            }
        }
    }
    else if (canonicalize == 1) {
        char canonicalize_buffer[term_size];
        for (size_t i = 0; i < num_terms; i++) {
            const char* normalized_kmer =
                canonicalize_kmer(query_8 + i, canonicalize_buffer, term_size);
            for (size_t j = 0; j < num_hashes; j++) {
                hashes[i * num_hashes + j] = XXH64(normalized_kmer, term_size, j);
            }
        }
    }
}

static inline
void counts_to_result(
    const std::vector<std::string>& file_names,
    const uint16_t* scores,
    std::vector<std::pair<uint16_t, std::string> >& result,
    size_t threshold, size_t num_results)
{
    // uninitialized index vector
    tlx::simple_vector<
        std::pair<uint16_t, uint32_t> > sorted_indices(file_names.size());

    // determine result size, from num_results and scores >= threshold, fill
    // index vector
    size_t count_threshold = 0;
    for (size_t i = 0; i < file_names.size(); ++i) {
        if (scores[i] >= threshold) {
            sorted_indices[count_threshold++] = std::make_pair(scores[i], i);
        }
    }
    num_results = std::min(num_results, count_threshold);

    std::partial_sort(
        sorted_indices.begin(), sorted_indices.begin() + num_results,
        sorted_indices.begin() + count_threshold,
        [&](auto v1, auto v2) {
            return std::tie(v2.first, v1.second) < std::tie(v1.first, v2.second);
        });

    result.resize(num_results);

    parallel_for(
        0, num_results, gopt_threads,
        [&](size_t i) {
            result[i] = std::make_pair(
                sorted_indices[i].first, file_names[sorted_indices[i].second]);
        });
}

void ClassicSearch::compute_counts(
    size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
#if __SSE2__
    auto expansion_128 = reinterpret_cast<const __m128i_u*>(s_expansion_128);
#endif
    uint64_t num_hashes = index_file_.num_hashes();

#if __SSE2__
    auto counts_128 = reinterpret_cast<__m128i_u*>(scores);
#else
    auto counts_64 = reinterpret_cast<uint64_t*>(scores);
#endif
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        const uint8_t* rows_8 = rows + i * buffer_size;
        for (size_t k = 0; k < size; k++) {
#if __SSE2__
            counts_128[k] = _mm_add_epi16(counts_128[k], expansion_128[rows_8[k]]);
#else
            counts_64[2 * k] += s_expansion[rows_8[k] & 0xF];
            counts_64[2 * k + 1] += s_expansion[rows_8[k] >> 4];
#endif
        }
    }
}

void ClassicSearch::aggregate_rows(size_t hashes_size, uint8_t* rows,
                                   const size_t size, size_t buffer_size)
{
    uint64_t num_hashes = index_file_.num_hashes();
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        uint8_t* rows_8 = rows + i * buffer_size;
        uint64_t* rows_64 = reinterpret_cast<uint64_t*>(rows_8);
        for (size_t j = 1; j < num_hashes; ++j) {
            uint8_t* rows_8_j = rows_8 + j * buffer_size;
            uint64_t* rows_64_j = reinterpret_cast<uint64_t*>(rows_8_j);

            assert((i * buffer_size + j * buffer_size) % 8 == 0);
            for (size_t k = 0; k < size / 8; ++k) {
                sLOG0 << i << hashes_size
                      << j << num_hashes << size
                      << reinterpret_cast<uint8_t*>(rows_64 + k) - rows
                      << reinterpret_cast<uint8_t*>(rows_64_j + k) - rows
                      << size * hashes_size;
                rows_64[k] &= rows_64_j[k];
            }
            size_t k = (size / 8) * 8;
            while (k < size) {
                rows_8[k] &= rows_8_j[k];
                k++;
            }
        }
    }
}

void ClassicSearch::search(
    const std::string& query,
    std::vector<std::pair<uint16_t, std::string> >& result,
    double threshold, size_t num_results)
{
    static constexpr bool debug = false;

    uint32_t term_size = index_file_.term_size();
    uint64_t page_size = index_file_.page_size();
    size_t counts_size = index_file_.counts_size();
    const std::vector<std::string>& file_names = index_file_.file_names();

    assert_exit(query.size() >= term_size,
                "query too short, needs to be at least "
                + std::to_string(term_size) + " characters long");
    assert_exit(query.size() - term_size < UINT16_MAX,
                "query too long, can not be longer than "
                + std::to_string(UINT16_MAX + term_size - 1) + " characters");

    size_t threshold_terms =
        std::ceil((query.size() - term_size + 1) * threshold);
    LOG0 << "threshold_terms = " << threshold_terms;

    timer_.active("hashes");
    num_results = num_results == 0 ? file_names.size()
                  : std::min(num_results, file_names.size());
    size_t scores_totalsize = counts_size;
    uint16_t* scores = allocate_aligned<uint16_t>(scores_totalsize, 16);
    std::vector<uint64_t> hashes;
    create_hashes(hashes, query);
    timer_.stop();

    size_t score_batch_size = 128;
    score_batch_size = std::max(score_batch_size, 8 * page_size);
    score_batch_size = std::min(score_batch_size, scores_totalsize);
    size_t score_batch_num = tlx::div_ceil(scores_totalsize, score_batch_size);
    LOG << "search()"
        << " score_batch_size=" << score_batch_size
        << " score_batch_num=" << score_batch_num
        << " scores_totalsize=" << scores_totalsize
        << " hashes.size=" << hashes.size();

    parallel_for(
        0, score_batch_num, gopt_threads,
        [&](size_t b) {
            Timer thr_timer;
            size_t score_begin = b * score_batch_size;
            size_t score_end = std::min((b + 1) * score_batch_size, scores_totalsize);
            size_t score_size = score_end - score_begin;
            LOG << "search()"
                << " score_begin=" << score_begin
                << " score_end=" << score_end
                << " score_size=" << score_size
                << " rows buffer=" << score_size * hashes.size();

            die_unless(score_begin % 8 == 0);
            score_begin = tlx::div_ceil(score_begin, 8);
            score_size = tlx::div_ceil(score_size, 8);
            size_t score_buffer_size = tlx::round_up(score_size, 8);

            // rows array: interleaved as
            // [ hash0[doc0, doc1, ..., doc(score_size)], hash1[doc0, ...], ...]
            uint8_t* rows = allocate_aligned<uint8_t>(
                score_buffer_size * hashes.size(), get_page_size());

            LOG << "read_from_disk";
            thr_timer.active("io");
            index_file_.read_from_disk(hashes, rows, score_begin, score_size,
                                       score_buffer_size);

            if (index_file_.num_hashes() != 1) {
                LOG << "aggregate_rows";
                thr_timer.active("and rows");
                aggregate_rows(hashes.size(), rows, score_size,
                               score_buffer_size);
            }

            LOG << "compute_counts";
            thr_timer.active("add rows");
            compute_counts(hashes.size(), scores + 8 * score_begin, rows,
                           score_size, score_buffer_size);

            deallocate_aligned(rows);

            timer_ += thr_timer;
        });

    timer_.active("sort results");
    counts_to_result(file_names, scores, result, threshold_terms, num_results);
    deallocate_aligned(scores);
    timer_.stop();
}

const uint64_t ClassicSearch::s_expansion[] = {
    0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833,
    281474976710656, 281474976710657, 281474976776192, 281474976776193,
    281479271677952, 281479271677953, 281479271743488, 281479271743489
};

const uint16_t ClassicSearch::s_expansion_128[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0,
    1, 0, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 0, 0, 1, 0, 0, 0, 0,
    0, 1, 0, 1, 0, 0, 0, 0,
    1, 1, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    1, 0, 1, 1, 0, 0, 0, 0,
    0, 1, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    1, 0, 0, 0, 1, 0, 0, 0,
    0, 1, 0, 0, 1, 0, 0, 0,
    1, 1, 0, 0, 1, 0, 0, 0,
    0, 0, 1, 0, 1, 0, 0, 0,
    1, 0, 1, 0, 1, 0, 0, 0,
    0, 1, 1, 0, 1, 0, 0, 0,
    1, 1, 1, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 1, 0, 0, 0,
    0, 1, 0, 1, 1, 0, 0, 0,
    1, 1, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 0, 0, 0,
    1, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0,
    1, 0, 0, 0, 0, 1, 0, 0,
    0, 1, 0, 0, 0, 1, 0, 0,
    1, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 1, 0, 0, 1, 0, 0,
    1, 0, 1, 0, 0, 1, 0, 0,
    0, 1, 1, 0, 0, 1, 0, 0,
    1, 1, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 1, 0, 1, 0, 0,
    1, 0, 0, 1, 0, 1, 0, 0,
    0, 1, 0, 1, 0, 1, 0, 0,
    1, 1, 0, 1, 0, 1, 0, 0,
    0, 0, 1, 1, 0, 1, 0, 0,
    1, 0, 1, 1, 0, 1, 0, 0,
    0, 1, 1, 1, 0, 1, 0, 0,
    1, 1, 1, 1, 0, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    1, 0, 0, 0, 1, 1, 0, 0,
    0, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 1, 0, 1, 1, 0, 0,
    1, 0, 1, 0, 1, 1, 0, 0,
    0, 1, 1, 0, 1, 1, 0, 0,
    1, 1, 1, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0,
    1, 0, 0, 1, 1, 1, 0, 0,
    0, 1, 0, 1, 1, 1, 0, 0,
    1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    1, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    1, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 0, 0, 0, 0, 1, 0,
    1, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0,
    1, 0, 1, 0, 0, 0, 1, 0,
    0, 1, 1, 0, 0, 0, 1, 0,
    1, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 1, 0,
    1, 0, 0, 1, 0, 0, 1, 0,
    0, 1, 0, 1, 0, 0, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 0,
    0, 0, 1, 1, 0, 0, 1, 0,
    1, 0, 1, 1, 0, 0, 1, 0,
    0, 1, 1, 1, 0, 0, 1, 0,
    1, 1, 1, 1, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    1, 0, 0, 0, 1, 0, 1, 0,
    0, 1, 0, 0, 1, 0, 1, 0,
    1, 1, 0, 0, 1, 0, 1, 0,
    0, 0, 1, 0, 1, 0, 1, 0,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 1, 0, 1, 0, 1, 0,
    1, 1, 1, 0, 1, 0, 1, 0,
    0, 0, 0, 1, 1, 0, 1, 0,
    1, 0, 0, 1, 1, 0, 1, 0,
    0, 1, 0, 1, 1, 0, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 0,
    0, 0, 1, 1, 1, 0, 1, 0,
    1, 0, 1, 1, 1, 0, 1, 0,
    0, 1, 1, 1, 1, 0, 1, 0,
    1, 1, 1, 1, 1, 0, 1, 0,
    0, 0, 0, 0, 0, 1, 1, 0,
    1, 0, 0, 0, 0, 1, 1, 0,
    0, 1, 0, 0, 0, 1, 1, 0,
    1, 1, 0, 0, 0, 1, 1, 0,
    0, 0, 1, 0, 0, 1, 1, 0,
    1, 0, 1, 0, 0, 1, 1, 0,
    0, 1, 1, 0, 0, 1, 1, 0,
    1, 1, 1, 0, 0, 1, 1, 0,
    0, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 0, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 0, 1, 1, 0,
    0, 0, 1, 1, 0, 1, 1, 0,
    1, 0, 1, 1, 0, 1, 1, 0,
    0, 1, 1, 1, 0, 1, 1, 0,
    1, 1, 1, 1, 0, 1, 1, 0,
    0, 0, 0, 0, 1, 1, 1, 0,
    1, 0, 0, 0, 1, 1, 1, 0,
    0, 1, 0, 0, 1, 1, 1, 0,
    1, 1, 0, 0, 1, 1, 1, 0,
    0, 0, 1, 0, 1, 1, 1, 0,
    1, 0, 1, 0, 1, 1, 1, 0,
    0, 1, 1, 0, 1, 1, 1, 0,
    1, 1, 1, 0, 1, 1, 1, 0,
    0, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 1, 1, 1, 1, 0,
    0, 1, 0, 1, 1, 1, 1, 0,
    1, 1, 0, 1, 1, 1, 1, 0,
    0, 0, 1, 1, 1, 1, 1, 0,
    1, 0, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    0, 1, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 0, 0, 1,
    0, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 0, 0, 0, 0, 1,
    1, 1, 1, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 1,
    1, 0, 0, 1, 0, 0, 0, 1,
    0, 1, 0, 1, 0, 0, 0, 1,
    1, 1, 0, 1, 0, 0, 0, 1,
    0, 0, 1, 1, 0, 0, 0, 1,
    1, 0, 1, 1, 0, 0, 0, 1,
    0, 1, 1, 1, 0, 0, 0, 1,
    1, 1, 1, 1, 0, 0, 0, 1,
    0, 0, 0, 0, 1, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 0, 1,
    0, 1, 0, 0, 1, 0, 0, 1,
    1, 1, 0, 0, 1, 0, 0, 1,
    0, 0, 1, 0, 1, 0, 0, 1,
    1, 0, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1,
    1, 1, 1, 0, 1, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1,
    1, 0, 0, 1, 1, 0, 0, 1,
    0, 1, 0, 1, 1, 0, 0, 1,
    1, 1, 0, 1, 1, 0, 0, 1,
    0, 0, 1, 1, 1, 0, 0, 1,
    1, 0, 1, 1, 1, 0, 0, 1,
    0, 1, 1, 1, 1, 0, 0, 1,
    1, 1, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 1, 0, 1,
    1, 0, 0, 0, 0, 1, 0, 1,
    0, 1, 0, 0, 0, 1, 0, 1,
    1, 1, 0, 0, 0, 1, 0, 1,
    0, 0, 1, 0, 0, 1, 0, 1,
    1, 0, 1, 0, 0, 1, 0, 1,
    0, 1, 1, 0, 0, 1, 0, 1,
    1, 1, 1, 0, 0, 1, 0, 1,
    0, 0, 0, 1, 0, 1, 0, 1,
    1, 0, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 1, 0, 1, 0, 1, 0, 1,
    0, 0, 1, 1, 0, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 0, 1,
    0, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 0, 1, 0, 1,
    0, 0, 0, 0, 1, 1, 0, 1,
    1, 0, 0, 0, 1, 1, 0, 1,
    0, 1, 0, 0, 1, 1, 0, 1,
    1, 1, 0, 0, 1, 1, 0, 1,
    0, 0, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 0, 1, 1, 0, 1,
    0, 1, 1, 0, 1, 1, 0, 1,
    1, 1, 1, 0, 1, 1, 0, 1,
    0, 0, 0, 1, 1, 1, 0, 1,
    1, 0, 0, 1, 1, 1, 0, 1,
    0, 1, 0, 1, 1, 1, 0, 1,
    1, 1, 0, 1, 1, 1, 0, 1,
    0, 0, 1, 1, 1, 1, 0, 1,
    1, 0, 1, 1, 1, 1, 0, 1,
    0, 1, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 1,
    0, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 1, 1,
    0, 1, 0, 0, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1,
    0, 0, 1, 0, 0, 0, 1, 1,
    1, 0, 1, 0, 0, 0, 1, 1,
    0, 1, 1, 0, 0, 0, 1, 1,
    1, 1, 1, 0, 0, 0, 1, 1,
    0, 0, 0, 1, 0, 0, 1, 1,
    1, 0, 0, 1, 0, 0, 1, 1,
    0, 1, 0, 1, 0, 0, 1, 1,
    1, 1, 0, 1, 0, 0, 1, 1,
    0, 0, 1, 1, 0, 0, 1, 1,
    1, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 1, 1, 0, 0, 1, 1,
    1, 1, 1, 1, 0, 0, 1, 1,
    0, 0, 0, 0, 1, 0, 1, 1,
    1, 0, 0, 0, 1, 0, 1, 1,
    0, 1, 0, 0, 1, 0, 1, 1,
    1, 1, 0, 0, 1, 0, 1, 1,
    0, 0, 1, 0, 1, 0, 1, 1,
    1, 0, 1, 0, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1,
    0, 0, 0, 1, 1, 0, 1, 1,
    1, 0, 0, 1, 1, 0, 1, 1,
    0, 1, 0, 1, 1, 0, 1, 1,
    1, 1, 0, 1, 1, 0, 1, 1,
    0, 0, 1, 1, 1, 0, 1, 1,
    1, 0, 1, 1, 1, 0, 1, 1,
    0, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 1, 1, 1,
    1, 0, 0, 0, 0, 1, 1, 1,
    0, 1, 0, 0, 0, 1, 1, 1,
    1, 1, 0, 0, 0, 1, 1, 1,
    0, 0, 1, 0, 0, 1, 1, 1,
    1, 0, 1, 0, 0, 1, 1, 1,
    0, 1, 1, 0, 0, 1, 1, 1,
    1, 1, 1, 0, 0, 1, 1, 1,
    0, 0, 0, 1, 0, 1, 1, 1,
    1, 0, 0, 1, 0, 1, 1, 1,
    0, 1, 0, 1, 0, 1, 1, 1,
    1, 1, 0, 1, 0, 1, 1, 1,
    0, 0, 1, 1, 0, 1, 1, 1,
    1, 0, 1, 1, 0, 1, 1, 1,
    0, 1, 1, 1, 0, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1, 1,
    1, 0, 0, 0, 1, 1, 1, 1,
    0, 1, 0, 0, 1, 1, 1, 1,
    1, 1, 0, 0, 1, 1, 1, 1,
    0, 0, 1, 0, 1, 1, 1, 1,
    1, 0, 1, 0, 1, 1, 1, 1,
    0, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 1, 1, 1,
    0, 0, 0, 1, 1, 1, 1, 1,
    1, 0, 0, 1, 1, 1, 1, 1,
    0, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

} // namespace cobs

/******************************************************************************/
