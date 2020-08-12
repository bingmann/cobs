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

#include <cobs/kmer.hpp>
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/query/compact_index/mmap_search_file.hpp>
#include <cobs/settings.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/misc.hpp>
#include <cobs/util/parallel_for.hpp>
#include <cobs/util/query.hpp>
#include <cobs/util/timer.hpp>

#include <algorithm>
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

ClassicSearch::ClassicSearch(std::shared_ptr<IndexSearchFile> index)
    : index_files_(
          std::vector<std::shared_ptr<IndexSearchFile> >{
              std::move(index)
          })
{ }

ClassicSearch::ClassicSearch(std::vector<std::shared_ptr<IndexSearchFile> > indices)
    : index_files_(std::move(indices)) { }

ClassicSearch::ClassicSearch(std::string path)
{
    if (file_has_header<ClassicIndexHeader>(path)) {
        index_files_.emplace_back(
            std::make_shared<ClassicIndexMMapSearchFile>(path));
    }
    else if (file_has_header<CompactIndexHeader>(path)) {
        index_files_.emplace_back(
            std::make_shared<CompactIndexMMapSearchFile>(path));
    }
    else {
        die("Could not open index path \"" << path << "\"");
    }
}

void ClassicSearch::create_hashes(
    std::vector<uint64_t>& hashes, const std::string& query,
    char* canonicalize_buffer,
    std::shared_ptr<IndexSearchFile> index_file_)
{
    uint32_t term_size = index_file_->term_size();
    size_t num_hashes = index_file_->num_hashes();
    uint8_t canonicalize = index_file_->canonicalize();

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
        for (size_t i = 0; i < num_terms; i++) {
            bool good =
                canonicalize_kmer(query_8 + i, canonicalize_buffer, term_size);

            if (!good) {
                die("Invalid DNA base pair in query string. "
                    "Only ACGT are allowed.");
            }

            for (size_t j = 0; j < num_hashes; j++) {
                hashes[i * num_hashes + j] = XXH64(
                    canonicalize_buffer, term_size, j);
            }
        }
    }
    else {
        die("Unknown canonicalize value " << unsigned(canonicalize));
    }
}

static inline
void counts_to_result(
    std::vector<std::shared_ptr<IndexSearchFile> >& index_files_,
    const uint16_t* scores,
    std::vector<std::pair<uint16_t, std::string> >& result,
    const std::vector<size_t>& thresholds,
    size_t num_results, size_t max_counts,
    const std::vector<size_t>& sum_doc_counts)
{
    // uninitialized index vector
    tlx::simple_vector<
        std::pair<uint16_t, std::pair<uint32_t, uint32_t> >
        > sorted_indices(sum_doc_counts.back());

    size_t count_threshold = 0;
    for (size_t i = 0; i < index_files_.size(); ++i) {
        for (size_t j = 0; j < index_files_[i]->file_names().size(); ++j) {
            size_t index = sum_doc_counts[i] + j;

            if (scores[index] >= thresholds[i]) {
                sorted_indices[count_threshold++] =
                    std::make_pair(scores[index], std::make_pair(i, j));
            }
        }
    }

    num_results = std::min(num_results, count_threshold);

    if (max_counts > 1)
    {
        std::partial_sort(
            sorted_indices.begin(), sorted_indices.begin() + num_results,
            sorted_indices.begin() + count_threshold,
            [&](auto v1, auto v2) {
                return (std::tie(v2.first, v1.second)
                        < std::tie(v1.first, v2.second));
            });
    }

    result.resize(num_results);

    for (size_t i = 0; i < num_results; ++i)
    {
        size_t index_id = sorted_indices[i].second.first;
        size_t document_id = sorted_indices[i].second.second;

        result[i] = std::make_pair(
            sorted_indices[i].first,
            index_files_[index_id]->file_names()[document_id]);
    }
}

void ClassicSearch::compute_counts(
    uint64_t num_hashes, size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
#if __SSE2__
    auto expansion_128 = reinterpret_cast<const __m128i_u*>(s_expansion_128);
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

void ClassicSearch::aggregate_rows(
    uint64_t num_hashes, size_t hashes_size, uint8_t* rows,
    const size_t size, size_t buffer_size)
{
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

    if (index_files_.empty())
        return;

    std::vector<size_t> sum_doc_counts(index_files_.size() + 1);

    // sum_doc_counts[i] - always rounded up to the next multiple of 8.  this
    // way we guarantee that the score array will always be 128 byte aligned
    sum_doc_counts[0] = 0;
    for (size_t i = 1; i <= index_files_.size(); ++i) {
        size_t counts_size = index_files_[i - 1]->counts_size();
        die_unless(counts_size % 8 == 0);
        sum_doc_counts[i] += sum_doc_counts[i - 1] + counts_size;
    }

    const size_t total_documents = sum_doc_counts[index_files_.size()];

    LOG << "ClassicSearch::search()"
        << " index_files_.size=" << index_files_.size()
        << " sum_doc_counts=" << sum_doc_counts
        << " total_documents=" << total_documents;

    uint16_t* score_list = allocate_aligned<uint16_t>(total_documents, 16);

    size_t total_hashes = 0;

    for (size_t file_num = 0; file_num < index_files_.size(); ++file_num)
    {
        auto& index_file_ = index_files_[file_num];

    uint32_t num_hashes = index_file_->num_hashes();
    uint32_t term_size = index_file_->term_size();
    uint64_t page_size = index_file_->page_size();
    size_t score_total_size = index_file_->counts_size();

    assert_exit(query.size() >= term_size,
                "query too short, needs to be at least "
                + std::to_string(term_size) + " characters long");
    assert_exit(query.size() - term_size < UINT16_MAX,
                "query too long, can not be longer than "
                + std::to_string(UINT16_MAX + term_size - 1) + " characters");

    timer_.active("hashes");
    std::vector<uint64_t> hashes;

    tlx::simple_vector<char> canonicalize_buffer(term_size);
    create_hashes(hashes, query, canonicalize_buffer.data(), index_file_);

    total_hashes += hashes.size();
    timer_.stop();

    size_t score_batch_size = 128;
    score_batch_size = std::max(score_batch_size, 8 * page_size);
    score_batch_size = std::min(score_batch_size, score_total_size);
    size_t score_batch_num = tlx::div_ceil(score_total_size, score_batch_size);
    uint16_t* score_start = score_list + sum_doc_counts[file_num];

    LOG << "ClassicSearch::search()"
        << " file_num=" << file_num
        << " num_hashes=" << num_hashes
        << " term_size=" << term_size
        << " page_size=" << page_size
        << " score_start=" << sum_doc_counts[file_num]
        << " score_total_size=" << score_total_size
        << " score_batch_size=" << score_batch_size
        << " score_batch_num=" << score_batch_num
        << " hashes.size=" << hashes.size();

    parallel_for(
        0, score_batch_num, gopt_threads,
        [&](size_t b) {
            Timer thr_timer;
            size_t score_begin = b * score_batch_size;
            size_t score_end =
                std::min((b + 1) * score_batch_size, score_total_size);
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
            index_file_->read_from_disk(
                hashes, rows, score_begin, score_size, score_buffer_size);

            if (num_hashes != 1) {
                LOG << "aggregate_rows";
                thr_timer.active("and rows");
                aggregate_rows(num_hashes, hashes.size(), rows,
                               score_size, score_buffer_size);
            }

            LOG << "compute_counts";
            thr_timer.active("add rows");
            compute_counts(num_hashes, hashes.size(),
                           score_start + 8 * score_begin, rows,
                           score_size, score_buffer_size);

            deallocate_aligned(rows);

            timer_ += thr_timer;
        });
    }

    std::vector<size_t> thresholds(index_files_.size());
    for (size_t i = 0; i < index_files_.size(); ++i) {
        thresholds[i] = std::ceil(
            threshold
            * (query.size() - index_files_[i]->term_size() + 1));
    }
    num_results = num_results == 0 ? total_documents
                  : std::min(num_results, total_documents);

    counts_to_result(index_files_, score_list, result, thresholds,
                     num_results, total_hashes, sum_doc_counts);
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
