#pragma once

#include <experimental/filesystem>
#include <experimental/filesystem>
#include <file/sample_header.hpp>
#include <file/util.hpp>
#include <file/compact_index_header.hpp>
#include <isi/classic_index.hpp>

namespace isi::compact_index {
    void create(const std::experimental::filesystem::path& in_dir, std::experimental::filesystem::path out_dir,
                size_t batch_size, size_t num_hashes, double false_positive_probability, uint64_t page_size = get_page_size());
    void create_folders(const std::experimental::filesystem::path& in_dir,
                        const std::experimental::filesystem::path& out_dir, uint64_t page_size = get_page_size());
    void create_compact_index_from_samples(const std::experimental::filesystem::path& in_dir, size_t batch_size, size_t num_hashes,
                                           double false_positive_probability, uint64_t page_size = get_page_size());
}
