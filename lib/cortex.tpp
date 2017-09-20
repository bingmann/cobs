#pragma once

#include <cortex.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <future>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>

#include "helpers.hpp"
#include "sample.hpp"

namespace cortex {

    template<unsigned int N>
    void serialize(const sample<N>& sample, const boost::filesystem::path& path) {
        boost::filesystem::create_directories(path.parent_path());
        std::ofstream ofs(path.string(), std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(sample.data().data()), kmer<N>::size * sample.data().size());
    }

    template<unsigned int N>
    void deserialize(sample<N>& sample, const boost::filesystem::path& path) {
        std::ifstream ifs(path.string(), std::ios::in | std::ios::binary);
        ifs.seekg(0, std::ios_base::end);
        int64_t size = ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);
        sample.data().resize(size / kmer<N>::size);
        ifs.read(reinterpret_cast<char*>(sample.data().data()), size);
    }

    template<typename size_type, typename ForwardIterator>
    size_type cast(ForwardIterator iter) {
        return *reinterpret_cast<const size_type*>(&(*iter));
    }

    template<typename ForwardIterator>
    void check_magic_number(ForwardIterator& iter) {
        std::string magic_word = "CORTEX";
        for (size_t i = 0; i < magic_word.size(); i++) {
            if(*iter != magic_word[i]) {
                throw std::invalid_argument("magic number does not match");
            }
            std::advance(iter, 1);
        }
    }

    template<typename ForwardIterator>
    header skip_header(ForwardIterator& iter) {
        header h{};
        check_magic_number(iter);
        h.version  = cast<uint32_t>(iter);
        std::advance(iter, 4);
        h.kmer_size  = cast<uint32_t>(iter);
        std::advance(iter, 4);
        h.num_words_per_kmer  = cast<uint32_t>(iter);
        std::advance(iter, 4);
        h.num_colors = cast<uint32_t>(iter);
        std::advance(iter, 16 * h.num_colors);
        for (size_t i = 0; i < h.num_colors; i++) {
            auto sample_name_length = cast<uint32_t>(iter);
            std::advance(iter, 4 + sample_name_length);
        }
        std::advance(iter, 16 * h.num_colors);
        for (size_t i = 0; i < h.num_colors; i++) {
            std::advance(iter, 12);
            auto length_graph_name = cast<uint32_t>(iter);
            std::advance(iter, 4 + length_graph_name);
        }
        check_magic_number(iter);
        return h;
    }

    template<typename ForwardIterator, unsigned int N>
    void read_sample(ForwardIterator iter, ForwardIterator end, header h, sample<N>& sample) {
        auto sample_data = reinterpret_cast<byte*>(sample.data().data());
        size_t num_bytes_per_kmer = 8 * h.num_words_per_kmer;

        while(iter != end) {
            std::copy(iter, std::next(iter, num_bytes_per_kmer), sample_data);
            std::advance(iter, num_bytes_per_kmer + 5 * h.num_colors);
            std::advance(sample_data, num_bytes_per_kmer);
        }
        t.next();

        std::sort(reinterpret_cast<uint64_t*>(&(*sample.data().begin())), reinterpret_cast<uint64_t*>(&(*sample.data().end())));

        auto c3 = std::chrono::high_resolution_clock::now();
    }

    template<unsigned int N>
    void process_file(const boost::filesystem::path& in_path, const boost::filesystem::path& out_path, sample<N>& s) {
        t.start();
        std::ifstream ifs(in_path.string(), std::ios::in | std::ios::binary);
        ifs.seekg(0, std::ios_base::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);
        v.resize(size);
        ifs.read(v.data(), size);
        t.next();
        auto iter = v.begin();
        auto h = skip_header(iter);
        s.data().resize(std::distance(iter, v.end()) / (8 * h.num_words_per_kmer + 5 * h.num_colors));

        read_sample(iter, v.end(), h, s);
        t.next();
        serialize(s, out_path);
        t.end();
    }

    template<unsigned int N>
    void process_all_in_directory(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir) {
        sample<N> sample;
        t.reset();
        size_t i = 0;
        for(boost::filesystem::recursive_directory_iterator end, it(in_dir); it != end; it++) {
            if(boost::filesystem::is_regular_file(*it)
               && it->path().extension() == ".ctx"
               && it->path().string().find("uncleaned") == std::string::npos
               && !boost::filesystem::exists(out_dir / it->path().stem().concat(".b"))) {
                try {
                    process_file(it->path(), out_dir / it->path().stem().concat(".b"), sample);
                    std::cout << std::left << std::setw(6) << i << " - " << it->path().string() << std::endl;
                    i++;
                } catch (const std::exception& e) {
                    std::cerr << it->path().string() << " - " << e.what() << std::endl;
                    t.end();
                }
            }
        }
        std::cout << t;
    }
}

