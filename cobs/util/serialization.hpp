/*******************************************************************************
 * cobs/util/serialization.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_SERIALIZATION_HEADER
#define COBS_UTIL_SERIALIZATION_HEADER
#pragma once

#include <cobs/util/fs.hpp>

#include <cassert>
#include <fstream>
#include <type_traits>

namespace cobs {

struct stream_metadata {
    uint64_t curr_pos;
    uint64_t end_pos;
};

static inline
stream_metadata get_stream_metadata(std::ifstream& is) {
    std::streamoff curr_pos = is.tellg();
    is.seekg(0, std::ios::end);
    std::streamoff end_pos = is.tellg();
    is.seekg(curr_pos, std::ios::beg);
    assert(is.good());
    assert(curr_pos >= 0);
    assert(end_pos >= 0);
    assert(end_pos >= curr_pos);
    return stream_metadata { (uint64_t)curr_pos, (uint64_t)end_pos };
}

template <typename T>
void read_file(const fs::path& path, std::vector<T>& v) {
    std::ifstream is(path.string(), std::ios::in | std::ios::binary);
    is.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    stream_metadata smd = get_stream_metadata(is);
    assert(smd.end_pos % sizeof(T) == 0);
    v.resize(smd.end_pos / sizeof(T));
    is.read(reinterpret_cast<char*>(v.data()), smd.end_pos);
}

template <typename T>
void write_pod(std::ostream& os, const T& t) {
    static_assert(std::is_pod<T>::value, "T must be POD");
    os.write(reinterpret_cast<const char*>(&t), sizeof(T));
}

static inline
void serialize(std::ostream& /* os */) { }

template <typename T, typename... Args>
void serialize(std::ostream& os, const T& t, const Args& ... args) {
    write_pod(os, t);
    serialize(os, args...);
}

template <typename T>
void read_pod(std::istream& is, T& t) {
    static_assert(std::is_pod<T>::value, "T must be POD");
    is.read(reinterpret_cast<char*>(&t), sizeof(T));
}

static inline
void deserialize(std::istream& /* is */) { }

template <typename T, typename... Args>
void deserialize(std::istream& is, T& t, Args& ... args) {
    read_pod(is, t);
    deserialize(is, args...);
}

} // namespace cobs

#endif // !COBS_UTIL_SERIALIZATION_HEADER

/******************************************************************************/
