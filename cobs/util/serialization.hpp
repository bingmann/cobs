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

#include <cassert>
#include <cobs/util/fs.hpp>
#include <fstream>

namespace cobs {

struct stream_metadata {
    uint64_t curr_pos;
    uint64_t end_pos;
};

stream_metadata get_stream_metadata(std::ifstream& ifs);

template <typename T>
void read_file(const fs::path& path, std::vector<T>& v);
template <typename T>
void write(std::ostream& ost, const T& t);
template <typename T, typename... Args>
void serialize(std::ostream& ost, const T& t, const Args& ... args);
template <typename T, typename... Args>
void deserialize(std::istream& ifs, T& t, Args& ... args);
void serialize(std::ostream& /*ost*/);
void deserialize(std::istream& /*ifs*/);

} // namespace cobs

namespace cobs {

template <typename T>
void read_file(const fs::path& path, std::vector<T>& v) {
    std::ifstream ifs(path.string(), std::ios::in | std::ios::binary);
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    stream_metadata smd = get_stream_metadata(ifs);
    assert(smd.end_pos % sizeof(T) == 0);
    v.resize(smd.end_pos / sizeof(T));
    ifs.read(reinterpret_cast<char*>(v.data()), smd.end_pos);
}

template <typename T>
void write(std::ostream& ost, const T& t) {
    ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
}

template <typename T, typename... Args>
void serialize(std::ostream& ost, const T& t, const Args& ... args) {
    ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
    serialize(ost, args...);
}

template <typename T, typename... Args>
void deserialize(std::istream& ifs, T& t, Args& ... args) {
    ifs.read(reinterpret_cast<char*>(&t), sizeof(T));
    deserialize(ifs, args...);
}

} // namespace cobs

#endif // !COBS_UTIL_SERIALIZATION_HEADER

/******************************************************************************/
