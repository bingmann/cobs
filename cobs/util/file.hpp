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

#include <cobs/util/fs.hpp>

#include <tlx/die.hpp>

namespace cobs {

template <class Header>
void serialize_header(std::ofstream& ofs, const fs::path& p, const Header& h) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    ofs.open(p.string(), std::ios::out | std::ios::binary);
    die_unless(ofs.good());
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
    die_unless(ifs.good());
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
        return false;
    }

    return true;
}

//! for a path, return the base file name without any extensions
static inline
std::string base_name(const fs::path& p) {
    std::string result = p.filename().string();
    std::string::size_type pos = result.find('.');
    if (pos == std::string::npos)
        return result;
    return result.substr(0, pos);
}

} // namespace cobs

#endif // !COBS_UTIL_FILE_HEADER

/******************************************************************************/
