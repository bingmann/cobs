/*******************************************************************************
 * cobs/construction/ranfold_index.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_CONSTRUCTION_RANFOLD_INDEX_HEADER
#define COBS_CONSTRUCTION_RANFOLD_INDEX_HEADER

#include <cobs/util/fs.hpp>

#include <xxhash.h>

/** The ranfold Inverted Signature Index without the space-saving improvements.
 *  This namespace provides methods for creation of this index. It can either be constructed from existing documents or
 *  with random dummy data for performance testing purposes.
 */
namespace cobs::ranfold_index {

/** Constructs the index by executing all necessary steps.
 */
void construct(const fs::path& in_dir, const fs::path& out_dir);

/** Constructs an index filled with random data.
 */
void construct_random(const fs::path& p,
                      uint64_t term_space, uint64_t num_term_hashes,
                      uint64_t doc_space_bits, uint64_t num_doc_hashes,
                      uint64_t num_documents, size_t document_size,
                      size_t seed);

} // namespace cobs::ranfold_index

#endif // !COBS_CONSTRUCTION_RANFOLD_INDEX_HEADER

/******************************************************************************/
