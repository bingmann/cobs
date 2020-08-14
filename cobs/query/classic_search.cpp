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

size_t ClassicSearch::num_documents() const
{
    size_t total = 0;
    for (const auto& p : index_files_) {
        total += p->file_names().size();
    }
    return total;
}

static inline
void create_hashes(
    std::vector<uint64_t>& hashes, const std::string& query,
    char* canonicalize_buffer,
    const std::shared_ptr<IndexSearchFile>& index_file)
{
    uint32_t term_size = index_file->term_size();
    size_t num_hashes = index_file->num_hashes();
    uint8_t canonicalize = index_file->canonicalize();

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

template <typename Score>
void counts_to_result(
    const std::vector<std::shared_ptr<IndexSearchFile> >& index_files,
    const Score* scores,
    std::vector<SearchResult>& result,
    const std::vector<size_t>& thresholds,
    size_t num_results, size_t max_counts,
    const std::vector<size_t>& sum_doc_counts)
{
    if (index_files.size() == 1)
    {
        const std::shared_ptr<IndexSearchFile>& index_file = index_files.front();

        // uninitialized index vector
        tlx::simple_vector<std::pair<Score, uint32_t> > sorted_indices(
            sum_doc_counts.back());

        size_t count_threshold = 0;
        for (size_t j = 0; j < index_file->file_names().size(); ++j) {
            if (scores[j] >= thresholds[0]) {
                sorted_indices[count_threshold++] =
                    std::make_pair(scores[j], j);
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
            size_t document_id = sorted_indices[i].second;

            result[i] = SearchResult(
                index_file->file_names()[document_id].c_str(),
                sorted_indices[i].first);
        }
    }
    else
    {
        // uninitialized index vector
        tlx::simple_vector<
            std::pair<Score, std::pair<uint16_t, uint32_t> >
            > sorted_indices(sum_doc_counts.back());

        size_t count_threshold = 0;
        for (size_t k = 0; k < index_files.size(); ++k) {
            for (size_t i = 0; i < index_files[k]->file_names().size(); ++i) {
                size_t index = sum_doc_counts[k] + i;

                if (scores[index] >= thresholds[k]) {
                    sorted_indices[count_threshold++] =
                        std::make_pair(scores[index], std::make_pair(k, i));
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

            result[i] = SearchResult(
                index_files[index_id]->file_names()[document_id].c_str(),
                sorted_indices[i].first);
        }
    }
}

/******************************************************************************/
// Score Expansion and Aggregation

bool classic_search_disable_8bit = false;
bool classic_search_disable_16bit = false;
bool classic_search_disable_32bit = false;

bool classic_search_disable_sse2 = false;

static inline
void compute_counts_u8_64(
    uint64_t num_hashes, size_t hashes_size, uint8_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size);

static inline
void compute_counts(
    uint64_t num_hashes, size_t hashes_size, uint8_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
    return compute_counts_u8_64(
        num_hashes, hashes_size, scores, rows, size, buffer_size);
}

static inline
void compute_counts_u16_64(
    uint64_t num_hashes, size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size);

static inline
void compute_counts_u16_128(
    uint64_t num_hashes, size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size);

static inline
void compute_counts(
    uint64_t num_hashes, size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
#if __SSE2__
    if (!classic_search_disable_sse2) {
        return compute_counts_u16_128(
            num_hashes, hashes_size, scores, rows, size, buffer_size);
    }
#endif
    return compute_counts_u16_64(
        num_hashes, hashes_size, scores, rows, size, buffer_size);
}

static inline
void compute_counts_u32_64(
    uint64_t num_hashes, size_t hashes_size, uint32_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size);

static inline
void compute_counts_u32_128(
    uint64_t num_hashes, size_t hashes_size, uint32_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size);

static inline
void compute_counts(
    uint64_t num_hashes, size_t hashes_size, uint32_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
#if __SSE2__
    if (!classic_search_disable_sse2) {
        return compute_counts_u32_128(
            num_hashes, hashes_size, scores, rows, size, buffer_size);
    }
#endif
    return compute_counts_u32_64(
        num_hashes, hashes_size, scores, rows, size, buffer_size);
}

/******************************************************************************/

static inline
void aggregate_rows(
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

template <typename Score>
void search_index_file(
    size_t file_num, const std::shared_ptr<IndexSearchFile>& index_file,
    const std::string& query, Score* score_list,
    size_t& total_hashes, const std::vector<size_t>& sum_doc_counts,
    Timer& timer)
{
    static constexpr bool debug = false;

    uint32_t num_hashes = index_file->num_hashes();
    uint32_t term_size = index_file->term_size();
    uint64_t page_size = index_file->page_size();
    size_t score_total_size = index_file->counts_size();

    if (query.size() - term_size >= std::numeric_limits<Score>::max()) {
        die("query too long, can not be longer than " <<
            std::numeric_limits<Score>::max() + term_size - 1 <<
            " characters");
    }

    timer.active("hashes");
    std::vector<uint64_t> hashes;

    tlx::simple_vector<char> canonicalize_buffer(term_size);
    create_hashes(hashes, query, canonicalize_buffer.data(), index_file);

    total_hashes += hashes.size();
    timer.stop();

    size_t score_batch_size = 128;
    score_batch_size = std::max(score_batch_size, 8 * page_size);
    score_batch_size = std::min(score_batch_size, score_total_size);
    size_t score_batch_num = tlx::div_ceil(score_total_size, score_batch_size);
    Score* score_start = score_list + sum_doc_counts[file_num];

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
            index_file->read_from_disk(
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

            timer += thr_timer;
        });
}

void ClassicSearch::search(
    const std::string& query,
    std::vector<SearchResult>& result,
    double threshold, size_t num_results)
{
    static constexpr bool debug = false;

    if (index_files_.empty())
        return;

    std::vector<size_t> sum_doc_counts(index_files_.size() + 1);

    uint32_t max_term_size = 0;

    // sum_doc_counts[i] - always rounded up to the next multiple of 8.  this
    // way we guarantee that the score array will always be 128 byte aligned
    sum_doc_counts[0] = 0;
    for (size_t i = 1; i <= index_files_.size(); ++i) {
        size_t counts_size = index_files_[i - 1]->counts_size();
        die_unless(counts_size % 8 == 0);
        sum_doc_counts[i] += sum_doc_counts[i - 1] + counts_size;

        // calculate largest term_size of indices and require query to be at
        // least that length
        uint32_t term_size = index_files_[i - 1]->term_size();
        max_term_size = std::max(max_term_size, term_size);
    }

    if (query.size() < max_term_size) {
        die("query too short, needs to be at least " <<
            max_term_size << " characters long");
    }

    const size_t total_documents = sum_doc_counts[index_files_.size()];

    LOG << "ClassicSearch::search()"
        << " index_files_.size=" << index_files_.size()
        << " sum_doc_counts=" << sum_doc_counts
        << " total_documents=" << total_documents;

    size_t total_hashes = 0;

    std::vector<size_t> thresholds(index_files_.size());
    for (size_t i = 0; i < index_files_.size(); ++i) {
        thresholds[i] = std::ceil(
            threshold
            * (query.size() - index_files_[i]->term_size() + 1));
    }
    num_results = num_results == 0 ? total_documents
                  : std::min(num_results, total_documents);

    if (!classic_search_disable_8bit &&
        query.size() - max_term_size < UINT8_MAX)
    {
        uint8_t* score_list = allocate_aligned<uint8_t>(total_documents, 16);

        for (size_t file_num = 0; file_num < index_files_.size(); ++file_num)
        {
            search_index_file(
                file_num, index_files_[file_num],
                query, score_list,
                total_hashes, sum_doc_counts, timer_);
        }

        counts_to_result(index_files_, score_list, result, thresholds,
                         num_results, total_hashes, sum_doc_counts);
    }
    else if (!classic_search_disable_16bit &&
             query.size() - max_term_size < UINT16_MAX)
    {
        uint16_t* score_list = allocate_aligned<uint16_t>(total_documents, 16);

        for (size_t file_num = 0; file_num < index_files_.size(); ++file_num)
        {
            search_index_file(
                file_num, index_files_[file_num],
                query, score_list,
                total_hashes, sum_doc_counts, timer_);
        }

        counts_to_result(index_files_, score_list, result, thresholds,
                         num_results, total_hashes, sum_doc_counts);
    }
    else if (!classic_search_disable_32bit &&
             query.size() - max_term_size < UINT32_MAX)
    {
        uint32_t* score_list = allocate_aligned<uint32_t>(total_documents, 16);

        for (size_t file_num = 0; file_num < index_files_.size(); ++file_num)
        {
            search_index_file(
                file_num, index_files_[file_num],
                query, score_list,
                total_hashes, sum_doc_counts, timer_);
        }

        counts_to_result(index_files_, score_list, result, thresholds,
                         num_results, total_hashes, sum_doc_counts);
    }
    else
    {
        die("query too long");
    }
}

/******************************************************************************/
// Score Expansion

//! expansion table from an 8-bit value (byte) and expanding it into one 64-bit
//! word containing 8x uint8_t indicators.
static const uint64_t s_expansion_u8_64[256] = {
    0x0000000000000000, 0x0000000000000001,
    0x0000000000000100, 0x0000000000000101,
    0x0000000000010000, 0x0000000000010001,
    0x0000000000010100, 0x0000000000010101,
    0x0000000001000000, 0x0000000001000001,
    0x0000000001000100, 0x0000000001000101,
    0x0000000001010000, 0x0000000001010001,
    0x0000000001010100, 0x0000000001010101,
    0x0000000100000000, 0x0000000100000001,
    0x0000000100000100, 0x0000000100000101,
    0x0000000100010000, 0x0000000100010001,
    0x0000000100010100, 0x0000000100010101,
    0x0000000101000000, 0x0000000101000001,
    0x0000000101000100, 0x0000000101000101,
    0x0000000101010000, 0x0000000101010001,
    0x0000000101010100, 0x0000000101010101,
    0x0000010000000000, 0x0000010000000001,
    0x0000010000000100, 0x0000010000000101,
    0x0000010000010000, 0x0000010000010001,
    0x0000010000010100, 0x0000010000010101,
    0x0000010001000000, 0x0000010001000001,
    0x0000010001000100, 0x0000010001000101,
    0x0000010001010000, 0x0000010001010001,
    0x0000010001010100, 0x0000010001010101,
    0x0000010100000000, 0x0000010100000001,
    0x0000010100000100, 0x0000010100000101,
    0x0000010100010000, 0x0000010100010001,
    0x0000010100010100, 0x0000010100010101,
    0x0000010101000000, 0x0000010101000001,
    0x0000010101000100, 0x0000010101000101,
    0x0000010101010000, 0x0000010101010001,
    0x0000010101010100, 0x0000010101010101,
    0x0001000000000000, 0x0001000000000001,
    0x0001000000000100, 0x0001000000000101,
    0x0001000000010000, 0x0001000000010001,
    0x0001000000010100, 0x0001000000010101,
    0x0001000001000000, 0x0001000001000001,
    0x0001000001000100, 0x0001000001000101,
    0x0001000001010000, 0x0001000001010001,
    0x0001000001010100, 0x0001000001010101,
    0x0001000100000000, 0x0001000100000001,
    0x0001000100000100, 0x0001000100000101,
    0x0001000100010000, 0x0001000100010001,
    0x0001000100010100, 0x0001000100010101,
    0x0001000101000000, 0x0001000101000001,
    0x0001000101000100, 0x0001000101000101,
    0x0001000101010000, 0x0001000101010001,
    0x0001000101010100, 0x0001000101010101,
    0x0001010000000000, 0x0001010000000001,
    0x0001010000000100, 0x0001010000000101,
    0x0001010000010000, 0x0001010000010001,
    0x0001010000010100, 0x0001010000010101,
    0x0001010001000000, 0x0001010001000001,
    0x0001010001000100, 0x0001010001000101,
    0x0001010001010000, 0x0001010001010001,
    0x0001010001010100, 0x0001010001010101,
    0x0001010100000000, 0x0001010100000001,
    0x0001010100000100, 0x0001010100000101,
    0x0001010100010000, 0x0001010100010001,
    0x0001010100010100, 0x0001010100010101,
    0x0001010101000000, 0x0001010101000001,
    0x0001010101000100, 0x0001010101000101,
    0x0001010101010000, 0x0001010101010001,
    0x0001010101010100, 0x0001010101010101,
    0x0100000000000000, 0x0100000000000001,
    0x0100000000000100, 0x0100000000000101,
    0x0100000000010000, 0x0100000000010001,
    0x0100000000010100, 0x0100000000010101,
    0x0100000001000000, 0x0100000001000001,
    0x0100000001000100, 0x0100000001000101,
    0x0100000001010000, 0x0100000001010001,
    0x0100000001010100, 0x0100000001010101,
    0x0100000100000000, 0x0100000100000001,
    0x0100000100000100, 0x0100000100000101,
    0x0100000100010000, 0x0100000100010001,
    0x0100000100010100, 0x0100000100010101,
    0x0100000101000000, 0x0100000101000001,
    0x0100000101000100, 0x0100000101000101,
    0x0100000101010000, 0x0100000101010001,
    0x0100000101010100, 0x0100000101010101,
    0x0100010000000000, 0x0100010000000001,
    0x0100010000000100, 0x0100010000000101,
    0x0100010000010000, 0x0100010000010001,
    0x0100010000010100, 0x0100010000010101,
    0x0100010001000000, 0x0100010001000001,
    0x0100010001000100, 0x0100010001000101,
    0x0100010001010000, 0x0100010001010001,
    0x0100010001010100, 0x0100010001010101,
    0x0100010100000000, 0x0100010100000001,
    0x0100010100000100, 0x0100010100000101,
    0x0100010100010000, 0x0100010100010001,
    0x0100010100010100, 0x0100010100010101,
    0x0100010101000000, 0x0100010101000001,
    0x0100010101000100, 0x0100010101000101,
    0x0100010101010000, 0x0100010101010001,
    0x0100010101010100, 0x0100010101010101,
    0x0101000000000000, 0x0101000000000001,
    0x0101000000000100, 0x0101000000000101,
    0x0101000000010000, 0x0101000000010001,
    0x0101000000010100, 0x0101000000010101,
    0x0101000001000000, 0x0101000001000001,
    0x0101000001000100, 0x0101000001000101,
    0x0101000001010000, 0x0101000001010001,
    0x0101000001010100, 0x0101000001010101,
    0x0101000100000000, 0x0101000100000001,
    0x0101000100000100, 0x0101000100000101,
    0x0101000100010000, 0x0101000100010001,
    0x0101000100010100, 0x0101000100010101,
    0x0101000101000000, 0x0101000101000001,
    0x0101000101000100, 0x0101000101000101,
    0x0101000101010000, 0x0101000101010001,
    0x0101000101010100, 0x0101000101010101,
    0x0101010000000000, 0x0101010000000001,
    0x0101010000000100, 0x0101010000000101,
    0x0101010000010000, 0x0101010000010001,
    0x0101010000010100, 0x0101010000010101,
    0x0101010001000000, 0x0101010001000001,
    0x0101010001000100, 0x0101010001000101,
    0x0101010001010000, 0x0101010001010001,
    0x0101010001010100, 0x0101010001010101,
    0x0101010100000000, 0x0101010100000001,
    0x0101010100000100, 0x0101010100000101,
    0x0101010100010000, 0x0101010100010001,
    0x0101010100010100, 0x0101010100010101,
    0x0101010101000000, 0x0101010101000001,
    0x0101010101000100, 0x0101010101000101,
    0x0101010101010000, 0x0101010101010001,
    0x0101010101010100, 0x0101010101010101
};

static inline
void compute_counts_u8_64(
    uint64_t num_hashes, size_t hashes_size, uint8_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
    auto counts_64 = reinterpret_cast<uint64_t*>(scores);
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        const uint8_t* rows_8 = rows + i * buffer_size;
        for (size_t k = 0; k < size; k++) {
            counts_64[k] += s_expansion_u8_64[rows_8[k]];
        }
    }
}

//! expansion table from an 4-bit value (nibble) and expanding it into one
//! 64-bit word containing 4x uint16_t indicators.
static const uint64_t s_expansion_u16_64[16] = {
    0x0000000000000, 0x0000000000001, 0x0000000010000, 0x0000000010001,
    0x0000100000000, 0x0000100000001, 0x0000100010000, 0x0000100010001,
    0x1000000000000, 0x1000000000001, 0x1000000010000, 0x1000000010001,
    0x1000100000000, 0x1000100000001, 0x1000100010000, 0x1000100010001
};

static inline
void compute_counts_u16_64(
    uint64_t num_hashes, size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
    auto counts_64 = reinterpret_cast<uint64_t*>(scores);
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        const uint8_t* rows_8 = rows + i * buffer_size;
        for (size_t k = 0; k < size; k++) {
            counts_64[2 * k] += s_expansion_u16_64[rows_8[k] & 0xF];
            counts_64[2 * k + 1] += s_expansion_u16_64[rows_8[k] >> 4];
        }
    }
}

//! expansion table from an 8-bit value (byte) and expanding it into one
//! 128-bit word containing 8x uint16_t indicators.
alignas(16) static const uint16_t s_expansion_u16_128[256 * 8] = {
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

static inline
void compute_counts_u16_128(
    uint64_t num_hashes, size_t hashes_size, uint16_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
#if __SSE2__
    auto expansion_128 = reinterpret_cast<const __m128i_u*>(s_expansion_u16_128);
    auto counts_128 = reinterpret_cast<__m128i_u*>(scores);
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        const uint8_t* rows_8 = rows + i * buffer_size;
        for (size_t k = 0; k < size; k++) {
            counts_128[k] = _mm_adds_epu16(counts_128[k], expansion_128[rows_8[k]]);
        }
    }
#endif
}

//! expansion table from an 2-bit value and expanding it into one 64-bit word
//! containing 2x uint32_t indicators.
static const uint64_t s_expansion_u32_64[4] = {
    0x0000000000000000, 0x0000000000000001,
    0x0000000100000000, 0x0000000100000001
};

static inline
void compute_counts_u32_64(
    uint64_t num_hashes, size_t hashes_size, uint32_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
    auto counts_64 = reinterpret_cast<uint64_t*>(scores);
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        const uint8_t* rows_8 = rows + i * buffer_size;
        for (size_t k = 0; k < size; k++) {
            counts_64[4 * k + 0] += s_expansion_u32_64[(rows_8[k] >> 0) & 0x3];
            counts_64[4 * k + 1] += s_expansion_u32_64[(rows_8[k] >> 2) & 0x3];
            counts_64[4 * k + 2] += s_expansion_u32_64[(rows_8[k] >> 4) & 0x3];
            counts_64[4 * k + 3] += s_expansion_u32_64[(rows_8[k] >> 6) & 0x3];
        }
    }
}

//! expansion table from an 8-bit value (byte) and expanding it into one
//! 128-bit word containing 8x uint32_t indicators.
alignas(32) static const uint32_t s_expansion_u32_128[16 * 4] = {
    0, 0, 0, 0,
    1, 0, 0, 0,
    0, 1, 0, 0,
    1, 1, 0, 0,
    0, 0, 1, 0,
    1, 0, 1, 0,
    0, 1, 1, 0,
    1, 1, 1, 0,
    0, 0, 0, 1,
    1, 0, 0, 1,
    0, 1, 0, 1,
    1, 1, 0, 1,
    0, 0, 1, 1,
    1, 0, 1, 1,
    0, 1, 1, 1,
    1, 1, 1, 1,
};

static inline
void compute_counts_u32_128(
    uint64_t num_hashes, size_t hashes_size, uint32_t* scores,
    const uint8_t* rows, size_t size, size_t buffer_size)
{
#if __SSE2__
    auto expansion_128 = reinterpret_cast<const __m128i_u*>(s_expansion_u32_128);
    auto counts_128 = reinterpret_cast<__m128i_u*>(scores);
    for (uint64_t i = 0; i < hashes_size; i += num_hashes) {
        const uint8_t* rows_8 = rows + i * buffer_size;
        for (size_t k = 0; k < size; k++) {
            counts_128[2 * k + 0] =
                _mm_add_epi32(counts_128[2 * k + 0], expansion_128[rows_8[k] & 0xF]);
            counts_128[2 * k + 1] =
                _mm_add_epi32(counts_128[2 * k + 1], expansion_128[rows_8[k] >> 4]);
        }
    }
#endif
}

} // namespace cobs

/******************************************************************************/
