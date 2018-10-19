/*******************************************************************************
 * cobs/construction/compact_index.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_CONSTRUCTION_COMPACT_INDEX_HEADER
#define COBS_CONSTRUCTION_COMPACT_INDEX_HEADER
#pragma once

#include <cobs/construction/classic_index.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/document_header.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

/** The compact Inverted Signature Index with the space-saving improvements.
 *  This namespace provides methods for creation of this index. It can either be constructed from existing documents or
 *  with random dummy data for performance testing purposes. The index uses different signature sizes to minimize
 *  space wastage.
 */
namespace cobs::compact_index {

/** Constructs the index by executing all necessary steps.
 *  First calls cobs::compact_index::create_folders to sort the documents by size and then
 *  split them into multiple batches. Afterwards combines these indices with a call
 *  to cobs::compact_index::construct_from_folders.
 */
void construct(const fs::path& in_dir, fs::path out_dir,
               size_t batch_size, size_t num_hashes, double false_positive_probability, uint64_t page_size = get_page_size());

/** Creates the folders used by the cobs::compact_index::create_from_folders.
 *  Sorts the documents by file size and then splits them into several directories.
 */
void create_folders(const fs::path& in_dir,
                    const fs::path& out_dir, uint64_t page_size = get_page_size());

/** Constructs the folders used by the cobs::compact_index::construct_from_documents.
 *  Sorts the documents by file size and then splits them into several directories.
 */
void construct_from_folders(const fs::path& in_dir, size_t batch_size, size_t num_hashes,
                            double false_positive_probability, uint64_t page_size = get_page_size());

void combine(const fs::path& in_dir, const fs::path& out_file, uint64_t page_size = get_page_size());

/** Constructs a dummy index filled with random data.
 */
void construct_dummy(const fs::path& p, size_t signature_size, size_t block_size, size_t num_hashes);

} // namespace cobs::compact_index

#endif // !COBS_CONSTRUCTION_COMPACT_INDEX_HEADER

/******************************************************************************/
