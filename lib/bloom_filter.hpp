#pragma once

#include <boost/filesystem.hpp>

namespace genome::bloom_filter {
    void process_all_in_directory(const boost::filesystem::path &in_dir, const boost::filesystem::path &out_dir,
                                  size_t bloom_filter_size, size_t block_size, size_t num_hashes);
    bool contains(const std::vector<byte>& signature, const kmer<31>& kmer, size_t bit_in_block);
}

#include "bloom_filter.tpp"
