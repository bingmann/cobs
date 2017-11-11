#pragma once

#include <vector>
#include <iostream>
#include <experimental/filesystem>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <functional>
#include <unistd.h>
#include <fstream>
#include <algorithm>


namespace isi {
    typedef unsigned char byte;

    struct stream_metadata {
        uint64_t curr_pos;
        uint64_t end_pos;
    };

    inline stream_metadata get_stream_metadata(std::ifstream& ifs) {
        std::streamoff curr_pos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        std::streamoff end_pos = ifs.tellg();
        ifs.seekg(curr_pos, std::ios::beg);
        assert(ifs.good());
        assert(curr_pos >= 0);
        assert(end_pos >= 0);
        assert(end_pos >= curr_pos);
        return {(uint64_t) curr_pos, (uint64_t) end_pos};
    };

    inline uint64_t get_page_size() {
        int page_size = getpagesize();
        assert(page_size > 0);
        assert(page_size == 4096); //todo check for experiments
        return (uint64_t) page_size;
    }

    template<class T>
    inline auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
        t.print(os);
        return os;
    }

    template<typename T>
    inline void read_file(const std::experimental::filesystem::path& path, std::vector<T>& v) {
        std::ifstream ifs(path.string(), std::ios::in | std::ios::binary);
        stream_metadata smd = get_stream_metadata(ifs);
        assert(smd.end_pos % sizeof(T) == 0);
        v.resize(smd.end_pos / sizeof(T));
        ifs.read(reinterpret_cast<char*>(v.data()), smd.end_pos);
    }

    template<typename T>
    inline void write(std::ostream& ost, const T& t) {
        ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    inline void serialize(std::ostream& /*ost*/) {}

    template<typename T, typename... Args>
    inline void serialize(std::ostream& ost, const T& t, const Args&... args) {
        ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
        serialize(ost, args...);
    }

    inline void deserialize(std::ifstream& /*ifs*/) {}

    template<typename T, typename... Args>
    inline void deserialize(std::ifstream& ifs, T& t, Args&... args) {
        ifs.read(reinterpret_cast<char*>(&t), sizeof(T));
        deserialize(ifs, args...);
    }

    inline void get_sorted_file_names(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, std::vector<std::experimental::filesystem::path>& paths) {
        std::experimental::filesystem::create_directories(out_dir);
        std::experimental::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy(it, end, std::back_inserter(paths));
        std::sort(paths.begin(), paths.end());
    }

    inline bool bulk_process_files(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, size_t bulk_size,
                                   const std::string& file_extension_in, const std::string& file_extension_out,
                                   const std::function<void(const std::vector<std::experimental::filesystem::path>&, const std::experimental::filesystem::path&)>& callback) {
        std::vector<std::experimental::filesystem::path> sorted_paths;
        get_sorted_file_names(in_dir, out_dir, sorted_paths);

        std::string first_filename;
        std::string last_filename;

        size_t j = 1;
        std::vector<std::experimental::filesystem::path> paths;
        for (size_t i = 0; i < sorted_paths.size(); i++) {
            if (std::experimental::filesystem::is_regular_file(sorted_paths[i]) && sorted_paths[i].extension() == file_extension_in) {
                std::string filename = sorted_paths[i].stem().string();
                if (first_filename.empty()) {
                    first_filename = filename;
                }
                last_filename = filename;
                paths.push_back(sorted_paths[i]);
            }
            if (paths.size() == bulk_size || (!paths.empty() && i + 1 == sorted_paths.size())) {
                std::experimental::filesystem::path out_file = out_dir / ("[" + first_filename + "-" + last_filename + "]" + file_extension_out);
                std::cout << std::left << std::setw(6) << j << "BEG " << out_file << std::flush;
                if (!std::experimental::filesystem::exists(out_file)) {
                    callback(paths, out_file);
                    std::cout << "\r" << std::left << std::setw(6) << j << "END " << out_file << std::endl;
                } else {
                    std::cout << "\r" << std::left << std::setw(6) << j << "EXI " << out_file << std::endl;
                }
                paths.clear();
                first_filename.clear();
                j++;
            }
        }
        return j < 3;
    }

    inline std::string random_sequence(size_t len) {
        std::array<char, 4> basepairs = {'A', 'C', 'G', 'T'};
        std::string result;
        std::srand(std::time(0));
        for (size_t i = 0; i < len; i++) {
            result += basepairs[std::rand() % 4];
        }
        return result;
    }

    inline double calc_signature_size_ratio(double num_hashes, double false_positive_probability) {
        double denominator = std::log(1 - std::pow(false_positive_probability, 1 / num_hashes));
        double result = -num_hashes / denominator;
        assert(result > 0);
        return result;
    }

    inline uint64_t calc_signature_size(size_t num_elements, double num_hashes, double false_positive_probability) {
        double signature_size_ratio = calc_signature_size_ratio(num_hashes, false_positive_probability);
        double result = std::ceil(num_elements * signature_size_ratio);
        assert(result >= 0);
        assert(result <= UINT64_MAX);
        return result;
    }


    inline void initialize_map() {
//    std::array<char, 4> chars = {'A', 'C', 'G', 'T'};
//    int b = 0;
//    for (byte i = 0; i < 4; i++) {
//        for (byte j = 0; j < 4; j++) {
//            for (byte k = 0; k < 4; k++) {
//                for (byte o = 0; o < 4; o++) {
//                    char c[4] = {chars[i], chars[j], chars[k], chars[o]};
//                    std::cout << "{" << *((unsigned int*) c) << ", " << b++ << "}, " << std::endl << std::flush;
//                    std::cout << "{" << (unsigned int) b++ << ", \"" << chars[i] << chars[j] << chars[k] << chars[o] << "\"}, " << std::endl << std::flush;
//                    m_bps_to_byte[chars_to_int(chars[i], chars[j], chars[k], chars[o])] = b++;
//                }
//            }
//        }
//    }
    }
    inline void initialize_map_server() {
        std::cout << "{";
        for (uint64_t i = 0; i < 16; i++) {
            uint64_t result = 0;
            result |= (i & 1);
            result |= (i & 2) << 15;
            result |= (i & 4) << 30;
            result |= (i & 8) << 45;
            std::cout << result << ", ";
        };
        std::cout << "}" << std::endl;
    }
};







