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

namespace cobs {

template <class Header>
void serialize_header(std::ofstream& ofs, const fs::path& p, const Header& h) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    ofs.open(p.string(), std::ios::out | std::ios::binary);
    h.serialize(ofs);
}

template <class Header>
void serialize_header(const fs::path& p, const Header& h) {
    std::ofstream ofs;
    serialize_header<Header>(ofs, p, h);
}

template <class Header>
Header deserialize_header(std::ifstream& ifs, const fs::path& p) {
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    ifs.open(p.string(), std::ios::in | std::ios::binary);
    Header h;
    h.deserialize(ifs);
    return h;
}

template <class Header>
Header deserialize_header(const fs::path& p) {
    std::ifstream ifs;
    return deserialize_header<Header>(ifs, p);
}

//! check if file has given header
template <class Header>
bool file_has_header(const fs::path& p) {
    if (!fs::is_regular_file(p))
        return false;

    try {
        deserialize_header<Header>(p);
    }
    catch (...) {
        std::cout << p.string() << " is not a " << typeid(Header).name() << std::endl;
        return false;
    }

    return true;
}

//! for a path, return the base file name without any extensions
static inline
std::string base_name(const fs::path& p) {
    std::string result = p.filename().string();
    auto comp = [](char c) {
                    return c == '.';
                };
    auto iter = std::find_if(result.rbegin(), result.rend(), comp) + 1;
    iter = std::find_if(iter, result.rend(), comp) + 1;
    return result.substr(0, std::distance(iter, result.rend()));
}

} // namespace cobs

#endif // !COBS_UTIL_FILE_HEADER

/******************************************************************************/
