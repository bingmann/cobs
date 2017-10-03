#pragma once

#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace genome {
    typedef unsigned char byte;

    template<class T>
    inline auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
        t.print(os);
        return os;
    }

    template<typename T>
    inline void read_file(const boost::filesystem::path& path, std::vector<T>& v) {
        std::ifstream ifs(path.string(), std::ios::in | std::ios::binary);
        ifs.seekg(0, std::ios_base::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);
        v.resize(size / sizeof(T));
        ifs.read(reinterpret_cast<char*>(v.data()), size);
    }

    template<typename T>
    inline void write(std::ostream& ost, const T& t) {
        ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    inline void serialize(std::ostream& ost) {}

    template<typename T, typename... Args>
    inline void serialize(std::ostream& ost, const T& t, const Args&... args) {
        ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
        serialize(ost, args...);
    }

    inline void deserialize(std::ifstream& ifs) {}

    template<typename T, typename... Args>
    inline void deserialize(std::ifstream& ifs, T& t, Args&... args) {
        ifs.read(reinterpret_cast<char*>(&t), sizeof(T));
        deserialize(ifs, args...);
    }

    inline void for_each_file_in_dir(const boost::filesystem::path& path,
                                     std::function<void(const boost::filesystem::path&)> callback) {
        boost::filesystem::directory_iterator end_itr;
        for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
            if (boost::filesystem::is_regular_file(itr->path()) && itr->path().filename().string() != ".DS_Store") {
                callback(itr->path());
            }
        }
    }

    inline std::vector<size_t> get_file_sizes_in_dir(const boost::filesystem::path& path) {
        std::vector<size_t> result;
        for_each_file_in_dir(path, [&](const auto& p) {
            result.push_back(boost::filesystem::file_size(p));
        });
        return result;
    }

    inline std::vector<std::string> get_files_in_dir(const boost::filesystem::path& path) {
        std::vector<std::string> result;
        for_each_file_in_dir(path, [&](const auto& p) {
            result.push_back(p.string());
        });
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
}
