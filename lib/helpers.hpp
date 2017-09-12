//
// Created by Florian Gauger on 09.09.2017.
//

#pragma once

#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/serialization.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>

typedef unsigned char byte;

template<class T>
inline auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
    t.print(os);
    return os;
}

template<class T>
inline void serialize(const T& t, const std::string& file_path) {
    std::ofstream ofs(file_path);
    boost::archive::binary_oarchive ar(ofs);
    ar << t;
    ofs.close();
}

template<class T>
inline T deserialize(const std::string& file_path) {
    std::ifstream ifs(file_path);
    boost::archive::binary_iarchive ia(ifs);
    T result;
    ia >> result;
    return result;
}

inline void for_each_file_in_dir(const boost::filesystem::path& path, std::function<void(const boost::filesystem::path&)> callback) {
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

