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
#include <cobs/util/file.hpp>
#include <cobs/util/misc.hpp>
#include <cobs/util/query.hpp>
#include <cobs/util/timer.hpp>
#include <numeric>
#include <string>
#include <vector>

#include <tlx/logger.hpp>
#include <tlx/math/div_ceil.hpp>

#include <xxhash.h>

namespace cobs::query {

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

void counts_to_result(const std::vector<std::string>& file_names,
                      const uint16_t* scores,
                      std::vector<std::pair<uint16_t, std::string> >& result,
                      size_t num_results) {
    std::vector<uint32_t> sorted_indices(file_names.size());
    std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
    std::partial_sort(
        sorted_indices.begin(), sorted_indices.begin() + num_results,
        sorted_indices.end(),
        [&](auto v1, auto v2) {
            return scores[v1] > scores[v2];
        });

    result.resize(num_results);
#pragma omp parallel for
    for (size_t i = 0; i < num_results; i++) {
        result[i] = std::make_pair(
            scores[sorted_indices[i]], file_names[sorted_indices[i]]);
    }
}

void ClassicSearch::compute_counts(size_t hashes_size, uint16_t* scores,
                                   const char* rows, size_t size) {
#ifndef NO_SIMD
    auto expansion_128 = reinterpret_cast<const __m128i_u*>(s_expansion_128);
#endif
    auto rows_b = reinterpret_cast<const uint8_t*>(rows);
    uint64_t num_hashes = index_file_.num_hashes();
    uint64_t bs = size;

#ifdef NO_SIMD
    auto counts_64 = reinterpret_cast<uint64_t*>(scores);
#else
    auto counts_128 = reinterpret_cast<__m128i_u*>(scores);
#endif
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        auto rows_8 = rows_b + i * bs;
        for (size_t k = 0; k < bs; k++) {
#ifdef NO_SIMD
            counts_64[2 * k] += s_expansion[rows_8[k] & 0xF];
            counts_64[2 * k + 1] += s_expansion[rows_8[k] >> 4];
#else
            counts_128[k] = _mm_add_epi16(counts_128[k], expansion_128[rows_8[k]]);
#endif
        }
    }
}

void ClassicSearch::aggregate_rows(size_t hashes_size, char* rows, size_t size) {
    uint64_t num_hashes = index_file_.num_hashes();
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        auto rows_8 = rows + i * size;
        auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
        for (size_t j = 1; j < num_hashes; j++) {
            auto rows_8_j = rows_8 + j * size;
            auto rows_64_j = reinterpret_cast<uint64_t*>(rows_8_j);
            size_t k = 0;
            while ((k + 1) * 8 <= size) {
                rows_64[k] &= rows_64_j[k];
                k++;
            }
            k = k * 8;
            while (k < size) {
                rows_8[k] &= rows_8_j[k];
                k++;
            }
        }
    }
}

void ClassicSearch::search(const std::string& query,
                           std::vector<std::pair<uint16_t, std::string> >& result,
                           size_t num_results) {
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

    timer_.active("hashes");
    num_results = num_results == 0 ? file_names.size()
                  : std::min(num_results, file_names.size());
    size_t scores_size = counts_size;
    uint16_t* scores = allocate_aligned<uint16_t>(scores_size, 16);
    std::vector<uint64_t> hashes;
    create_hashes(hashes, query);

    size_t score_batch_size = 128;
    score_batch_size = std::max(score_batch_size, 8 * page_size);
    size_t score_batch_num = tlx::div_ceil(scores_size, score_batch_size);

#pragma omp parallel for
    for (size_t b = 0; b < score_batch_num; ++b) {
        Timer thr_timer;
        size_t score_begin = b * score_batch_size;
        size_t score_end = std::min((b + 1) * score_batch_size, scores_size);
        size_t score_size = score_end - score_begin;
        LOG << "search() score_begin=" << score_begin
            << " score_end=" << score_end
            << " score_size=" << score_size
            << " rows buffer=" << score_size * hashes.size();

        die_unless(score_begin % 8 == 0);
        score_begin = tlx::div_ceil(score_begin, 8);
        score_size = tlx::div_ceil(score_size, 8);

        // rows array: interleaved as
        // [ hash0[doc0, doc1, ..., doc(score_size)], hash1[doc0, ...], ...]
        char* rows = allocate_aligned<char>(score_size * hashes.size(), get_page_size());

        LOG << "read_from_disk";
        thr_timer.active("io");
        index_file_.read_from_disk(hashes, rows, score_begin, score_size);

        LOG << "aggregate_rows";
        thr_timer.active("and rows");
        aggregate_rows(hashes.size(), rows, score_size);

        LOG << "compute_counts";
        thr_timer.active("add rows");
        compute_counts(hashes.size(), scores + 8 * score_begin, rows, score_size);

        deallocate_aligned(rows);

#pragma omp critical
        timer_ += thr_timer;
    }

    timer_.active("sort results");
    counts_to_result(file_names, scores, result, num_results);
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

} // namespace cobs::query

/******************************************************************************/
