/*******************************************************************************
 * cobs/construction/classic_index.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_CONSTRUCTION_CLASSIC_INDEX_HEADER
#define COBS_CONSTRUCTION_CLASSIC_INDEX_HEADER
#pragma once

#include <cobs/file/classic_index_header.hpp>
#include <cobs/kmer.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

/** The classic Inverted Signature Index without the space-saving improvements.
 *  This namespace provides methods for creation of this index. It can either be constructed from existing documents or
 *  with random dummy data for performance testing purposes.
 */
namespace cobs::classic_index  {

/** Constructs the index by executing all necessary steps.
 *  First calls cobs::classic_index::construct_from_documents to construct multiple small indices.
 *  Afterwards combines these indices with calls to cobs::classic_index::combine until only one index remains.
 */
void construct(const fs::path& in_dir, const fs::path& out_dir,
               uint64_t batch_size, uint64_t num_hashes, double false_positive_probability);

/** Constructs multiple small indices from document files.
 */
void construct_from_documents(const fs::path& in_dir, const fs::path& out_dir,
                              uint64_t signature_size, uint64_t num_hashes, uint64_t batch_size);

/** Combines multiple indices into one or more bigger indices.
 */
bool combine(const fs::path& in_dir, const fs::path& out_dir, uint64_t batch_size);

/** Constructs the hash used by the signatures.
 */
void create_hashes(const void* input, size_t len, uint64_t signature_size, uint64_t num_hashes,
                   const std::function<void(uint64_t)>& callback);

/** Constructs a dummy index filled with random data.
 */
void construct_dummy(const fs::path& p, uint64_t signature_size, uint64_t block_size, uint64_t num_hashes, size_t seed);

} // namespace cobs::classic_index

#endif // !COBS_CONSTRUCTION_CLASSIC_INDEX_HEADER

/******************************************************************************/
