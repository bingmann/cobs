/*******************************************************************************
 * cobs/construction/classic_index.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_CONSTRUCTION_CLASSIC_INDEX_HEADER
#define COBS_CONSTRUCTION_CLASSIC_INDEX_HEADER

#include <cobs/document_list.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/misc.hpp>

/*!
 * The classic Inverted Signature Index without the space-saving improvements.
 *
 * This namespace provides methods for creation of this index. It can either be
 * constructed from existing documents or with random dummy data for performance
 * testing purposes.
 */
namespace cobs {

/*!
 * Parameters for classic index construction.
 */
struct ClassicIndexParameters {
    //! length of terms / k-mers
    unsigned term_size = 31;
    //! canonicalization flag for base pairs
    uint8_t canonicalize = 0;
    //! number of hash functions, provided by user
    unsigned num_hashes = 1;
    //! false positive rate, provided by user
    double false_positive_rate = 0.3;
    //! signature size, either provided by user or calculated from
    //! false_positive_rate if zero.
    uint64_t signature_size = 0;
    //! memory to use bytes to create index
    uint64_t mem_bytes = get_memory_size(80);
    //! number of threads to use
    size_t num_threads = gopt_threads;
    //! log prefix
    std::string log_prefix;
};

/*!
 * Constructs the index by executing all necessary steps.
 *
 * First calls cobs::classic_construct_from_documents() to construct multiple
 * small indices.  Afterwards combines these indices with calls to
 * cobs::classic_combine until only one index remains.
 */
void classic_construct(
    const DocumentList& filelist, const fs::path& out_dir,
    const fs::path& tmp_path, ClassicIndexParameters index_params);

/*!
 * Constructs multiple small indices from document files.
 */
void classic_construct_from_documents(
    const DocumentList& doc_list, const fs::path& out_dir,
    const ClassicIndexParameters& index_params);

/*!
 * Combines multiple indices into one or more bigger indices.
 */
bool classic_combine(
    const fs::path& in_dir, const fs::path& out_dir, fs::path& result_file,
    uint64_t mem_bytes, size_t num_threads);

/*!
 * Constructs a classic index filled with random data.
 */
void classic_construct_random(
    const fs::path& out_file, uint64_t signature_size,
    uint64_t num_documents, size_t document_size,
    uint64_t num_hashes, size_t seed);

} // namespace cobs

#endif // !COBS_CONSTRUCTION_CLASSIC_INDEX_HEADER

/******************************************************************************/
