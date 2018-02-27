#pragma once

#include <experimental/filesystem>
#include <experimental/filesystem>
#include <cobs/file/sample_header.hpp>
#include <cobs/util/file.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/construction/classic_index.hpp>

/** The compact Inverted Signature Index with the space-saving improvements.
 *  This namespace provides methods for creation of this index. It can either be created from existing samples or
 *  with random dummy data for performance testing purposes. The index uses different signature sizes to minimize
 *  space wastage.
 */
namespace cobs::compact_index {

    /** Creates the index by executing all necessary steps.
     *  First calls cobs::compact_index::create_folders to sort the samples by size and then
     *  split them into multiple batches. Afterwards combines these indices with a call
     *  to cobs::compact_index::create_from_folders.
     */
    void create(const std::experimental::filesystem::path& in_dir, std::experimental::filesystem::path out_dir,
                size_t batch_size, size_t num_hashes, double false_positive_probability, uint64_t page_size = get_page_size());

    /** Creates the folders used by the cobs::compact_index::create_from_folders.
     *  Sorts the samples by file size and then splits them into several directories.
     */
    void create_folders(const std::experimental::filesystem::path& in_dir,
                        const std::experimental::filesystem::path& out_dir, uint64_t page_size = get_page_size());

    /** Creates the folders used by the cobs::compact_index::create_from_samples.
     *  Sorts the samples by file size and then splits them into several directories.
     */
    void create_from_folders(const std::experimental::filesystem::path& in_dir, size_t batch_size, size_t num_hashes,
                                           double false_positive_probability, uint64_t page_size = get_page_size());


    void combine(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_file, uint64_t page_size = get_page_size());

    /** Creates a dummy index filled with random data.
     */
    void create_dummy(const std::experimental::filesystem::path& p, size_t signature_size, size_t block_size, size_t num_hashes);
}
