/*******************************************************************************
 * cobs/util/file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_FILE_HEADER
#define COBS_UTIL_FILE_HEADER

#include <algorithm>
#include <fstream>
#include <iostream>

#include <cobs/file/header.hpp>
#include <cobs/util/fs.hpp>

namespace cobs::file {

template <class T>
void serialize_header(std::ofstream& ofs, const fs::path& p, const T& h);

template <class T>
void serialize_header(const fs::path& p, const T& h);

template <class T>
T deserialize_header(std::ifstream& ifs, const fs::path& p);

template <class T>
T deserialize_header(const fs::path& p);

template <class T>
bool file_is(const fs::path& p);

std::string file_name(const fs::path& p);

} // namespace cobs::file

namespace cobs::file {

template <class T>
void serialize_header(std::ofstream& ofs, const fs::path& p, const T& h) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    ofs.open(p.string(), std::ios::out | std::ios::binary);
    Header<T>::serialize(ofs, h);
}

template <class T>
void serialize_header(const fs::path& p, const T& h) {
    std::ofstream ofs;
    serialize_header<T>(ofs, p, h);
}

template <class T>
T deserialize_header(std::ifstream& ifs, const fs::path& p) {
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    ifs.open(p.string(), std::ios::in | std::ios::binary);
    T h;
    Header<T>::deserialize(ifs, h);
    return h;
}

template <class T>
T deserialize_header(const fs::path& p) {
    std::ifstream ifs;
    return deserialize_header<T>(ifs, p);
}

template <class T>
bool file_is(const fs::path& p) {
    bool result = fs::is_regular_file(p);
    if (result) {
        try {
            deserialize_header<T>(p);
        } catch (...) {
            std::cout << p.string() << " is not a " << typeid(T).name() << std::endl;
            result = false;
        }
    }

    return result;
}

} // namespace cobs::file

#endif // !COBS_UTIL_FILE_HEADER

/******************************************************************************/
