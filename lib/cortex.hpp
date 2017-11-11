#pragma once

#include <string>
#include <vector>
#include "sample.hpp"
#include "timer.hpp"
#include <experimental/filesystem>

namespace isi::cortex {
    struct header {
        uint32_t version;
        uint32_t kmer_size;
        uint32_t num_words_per_kmer;
        uint32_t num_colors;
    };

    std::vector<char> v;
    timer t;

    template<typename size_type, typename ForwardIterator>
    size_type cast(ForwardIterator iter);

    template<typename ForwardIterator>
    void check_magic_number(ForwardIterator& iter);

    template<typename ForwardIterator>
    header skip_header(ForwardIterator& iter);

    template<typename ForwardIterator, unsigned int N>
    void read_sample(ForwardIterator iter, ForwardIterator end, header h, sample<N>& sample);

    template<unsigned int N>
    void deserialize(sample<N>& sample, const std::experimental::filesystem::path& path);

    template<unsigned int N>
    void process_file(const std::experimental::filesystem::path& in_path, const std::experimental::filesystem::path& out_path, sample<N>& s);

    template<unsigned int N>
    void process_all_in_directory(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir);
}

#include "cortex.tpp"
