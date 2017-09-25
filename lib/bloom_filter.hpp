#pragma once

#include <boost/filesystem.hpp>

namespace genome::bloom_filter {
    template<size_t BLOOM_FILTER_SIZE, size_t BLOCK_SIZE, size_t NUM_HASHES>
    void process_all_in_directory(const boost::filesystem::path &in_dir, const boost::filesystem::path &out_dir, size_t batch_size);
}

#include "bloom_filter.tpp"
