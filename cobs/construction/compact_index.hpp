/*******************************************************************************
 * cobs/construction/compact_index.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_CONSTRUCTION_COMPACT_INDEX_HEADER
#define COBS_CONSTRUCTION_COMPACT_INDEX_HEADER

#include <cobs/util/fs.hpp>
#include <cobs/util/misc.hpp>

/*!
 * The compact Inverted Signature Index with the space-saving improvements.
 * This namespace provides methods for creation of this index. It can either be
 * constructed from existing documents or with random dummy data for performance
 * testing purposes. The index uses different signature sizes to minimize space
 * wastage.
 */
namespace cobs::compact_index {

/*!
 * Constructs the folders used by the
 * cobs::compact_index::construct_from_documents.  Sorts the documents by file
 * size and then splits them into several directories.
 */
void construct_from_documents(
    const fs::path& in_dir, const fs::path& index_dir,
    size_t batch_size, size_t num_hashes,
    double false_positive_probability, uint64_t page_size = get_page_size());

void combine_into_compact(const fs::path& in_dir, const fs::path& out_file,
                          uint64_t page_size = get_page_size());

/*!
 * Constructs a dummy index filled with random data.
 */
void construct_dummy(const fs::path& p, size_t signature_size,
                     size_t row_size, size_t num_hashes);

} // namespace cobs::compact_index

#endif // !COBS_CONSTRUCTION_COMPACT_INDEX_HEADER

/******************************************************************************/
