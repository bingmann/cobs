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
    unsigned num_hashes;
    //! false positive rate, provided by user
    double false_positive_rate = 0;
    //! signature size, either provided by user or calculated from FPR
    uint64_t signature_size = 0;
    //! batch size in bytes to process per thread
    uint64_t batch_bytes = 128 * 1024 * 1024llu;
    //! number of documents in a batch. calculated by construct() from
    //! batch_bytes and the document's size, passed from compact_index
    size_t batch_size = 0;
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
    ClassicIndexParameters index_params);

/*!
 * Constructs multiple small indices from document files.
 */
void classic_construct_from_documents(
    const DocumentList& doc_list, const fs::path& out_dir,
    ClassicIndexParameters index_params);

/*!
 * Combines multiple indices into one or more bigger indices.
 */
bool classic_combine(
    const fs::path& in_dir, const fs::path& out_dir, uint64_t batch_size);

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
