#pragma once

#include <experimental/filesystem>
#include <isi/util/processing.hpp>
#include <isi/kmer.hpp>
#include <isi/util/timer.hpp>
#include <isi/file/classic_index_header.hpp>

/** The classic Inverted Signature Index without the space-saving improvements.
 *  This namespace provides methods for creation of this index. It can either be created from existing samples or
 *  with random dummy data for performance testing purposes.
 */
namespace isi::classic_index  {

    /** Creates the index by executing all necessary steps.
     *  First calls isi::classic_index::create_from_samples to create multiple small indices.
     *  Afterwards combines these indices with calls to isi::classic_index::combine until only one index remains.
     */
    void create(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                uint64_t batch_size, uint64_t num_hashes, double false_positive_probability);


    /** Creates multiple small indices from sample files.
     */
    void create_from_samples(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                             uint64_t signature_size, uint64_t num_hashes, uint64_t batch_size);

    /** Combines multiple indices into one or more bigger indices.
     */
    bool combine(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, uint64_t batch_size);

    /** Creates the hash used by the signatures.
     */
    void create_hashes(const void* input, size_t len, uint64_t signature_size, uint64_t num_hashes,
                       const std::function<void(uint64_t)>& callback);

    /** Creates a dummy index filled with random data.
     */
    void create_dummy(const std::experimental::filesystem::path& p, uint64_t signature_size, uint64_t block_size, uint64_t num_hashes);
}
