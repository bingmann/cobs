#pragma once

#include <experimental/filesystem>
#include <util.hpp>
#include <kmer.hpp>
#include <timer.hpp>
#include <file/classic_index_header.hpp>

namespace isi::classic_index  {
    void create(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                size_t signature_size, size_t block_size, size_t num_hashes, size_t batch_size);
    void create_from_samples(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                             size_t signature_size, size_t block_size, size_t num_hashes);
    bool combine(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                 size_t batch_size, size_t signature_size, size_t num_hashes);
    void create_hashes(const void* input, size_t len, size_t signature_size, size_t num_hashes,
                       const std::function<void(size_t)>& callback);
    void create_dummy(const std::experimental::filesystem::path& p, size_t signature_size, size_t block_size, size_t num_hashes);
}
