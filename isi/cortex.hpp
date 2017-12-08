#pragma once

#include <string>
#include <vector>
#include <experimental/filesystem>

#include <isi/sample.hpp>
#include <isi/util/timer.hpp>
#include <isi/util/file.hpp>
#include <iomanip>
#include <cstring>

namespace isi::cortex {
    struct header {
        uint32_t version;
        uint32_t kmer_size;
        uint32_t num_words_per_kmer;
        uint32_t num_colors;
        std::string name;
    };

    std::vector<char> v;
    timer t;

    template<typename size_type, typename ForwardIterator>
    size_type cast(ForwardIterator iter) {
        return *reinterpret_cast<const size_type*>(&(*iter));
    }

    template<typename ForwardIterator>
    void check_magic_number(ForwardIterator& iter) {
        std::string magic_word = "CORTEX";
        for (size_t i = 0; i < magic_word.size(); i++) {
            if (*iter != magic_word[i]) {
                throw std::invalid_argument("magic number does not match");
            }
            std::advance(iter, 1);
        }
    }

    template<typename ForwardIterator>
    header skip_header(ForwardIterator& iter) {
        header h{};
        check_magic_number(iter);
        h.version = cast<uint32_t>(iter);
        if (h.version != 6) {
            throw std::invalid_argument("invalid .ctx file version (" + std::to_string(h.version));
        }
        std::advance(iter, 4);
        h.kmer_size = cast<uint32_t>(iter);
        assert(h.kmer_size == 31);
        std::advance(iter, 4);
        h.num_words_per_kmer = cast<uint32_t>(iter);
        std::advance(iter, 4);
        h.num_colors = cast<uint32_t>(iter);
        if (h.num_colors != 1) {
            throw std::invalid_argument("invalid number of colors (" + std::to_string(h.num_colors) + "), should be 1");
        }
        std::advance(iter, 16 * h.num_colors);
        for (size_t i = 0; i < h.num_colors; i++) {
            auto sample_name_length = cast<uint32_t>(iter);
            std::advance(iter, 4);
            h.name.assign(iter, iter + sample_name_length);
            std::advance(iter, sample_name_length);
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
    void read_sample(ForwardIterator iter, ForwardIterator end, header h, sample <N>& sample) {
        auto sample_data = reinterpret_cast<uint8_t*>(sample.data().data());
        size_t num_uint8_ts_per_kmer = 8 * h.num_words_per_kmer;

        while (iter != end) {
            std::copy(iter, std::next(iter, num_uint8_ts_per_kmer), sample_data);
            std::advance(iter, num_uint8_ts_per_kmer + 5 * h.num_colors);
            std::advance(sample_data, num_uint8_ts_per_kmer);
        }

        t.active("sort");
        std::sort(reinterpret_cast<uint64_t*>(&(*sample.data().begin())),
                  reinterpret_cast<uint64_t*>(&(*sample.data().end())));
    }

    template<unsigned int N>
    void process_file(const std::experimental::filesystem::path& in_path, const std::experimental::filesystem::path& out_path, sample <N>& s) {
        t.active("read");
        read_file(in_path, v);
        if (!v.empty()) {
            t.active("iter");
            auto iter = v.begin();
            auto h = skip_header(iter);
            s.data().resize(std::distance(iter, v.end()) / (8 * h.num_words_per_kmer + 5 * h.num_colors));

            read_sample(iter, v.end(), h, s);
            t.active("write");
            file::serialize<31>(out_path, s, h.name);
        }
        t.stop();
    }

    template<unsigned int N>
    void process_all_in_directory(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir) {
        sample<N> sample;
        t.reset();
        size_t i = 0;
        for (std::experimental::filesystem::recursive_directory_iterator end, it(in_dir); it != end; it++) {
            std::experimental::filesystem::path out_path = out_dir / it->path().stem().concat(file::sample_header::file_extension);
            if (std::experimental::filesystem::is_regular_file(*it)
                && it->path().extension().string() == ".ctx"
                && it->path().string().find("uncleaned") == std::string::npos
                //todo remove and add proper handling for corrupted .ctx files
                && it->path().string().find("ERR1458685") == std::string::npos
                && it->path().string().find("ERR1103853") == std::string::npos
                && !std::experimental::filesystem::exists(out_path)) {
                std::cout << "BE - " << std::setfill('0') << std::setw(7) << i << " - " << it->path().string() << std::flush;
                bool success = true;
                try {
                    process_file(it->path(), out_path, sample);
                } catch (const std::exception& e) {
                    std::cerr << it->path().string() << " - " << e.what() << " - " << std::strerror(errno) << std::endl;
                    success = false;
                    t.stop();
                }
                std::cout << "\r" << (success ? "OK" : "ER") << " - " << std::setfill('0') << std::setw(7) << i << " - " << it->path().string() << std::endl;
                i++;
            }
        }
        std::cout << t;
    }
}

