/*******************************************************************************
 * cobs/util/serialization.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_SERIALIZATION_HEADER
#define COBS_UTIL_SERIALIZATION_HEADER

#include <cobs/util/fs.hpp>

#include <fstream>
#include <type_traits>

#include <tlx/die.hpp>

namespace cobs {

struct StreamMetadata {
    uint64_t curr_pos;
    uint64_t end_pos;
};

static inline
StreamMetadata get_stream_metadata(std::ifstream& is) {
    std::streamoff curr_pos = is.tellg();
    is.seekg(0, std::ios::end);
    std::streamoff end_pos = is.tellg();
    is.seekg(curr_pos, std::ios::beg);
    die_unless(is.good());
    die_unless(curr_pos >= 0);
    die_unless(end_pos >= 0);
    die_unless(end_pos >= curr_pos);
    return StreamMetadata { (uint64_t)curr_pos, (uint64_t)end_pos };
}

template <typename T>
void read_file(const fs::path& path, std::vector<T>& v) {
    std::ifstream is(path.string(), std::ios::in | std::ios::binary);
    is.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    StreamMetadata smd = get_stream_metadata(is);
    die_unless(smd.end_pos % sizeof(T) == 0);
    v.resize(smd.end_pos / sizeof(T));
    is.read(reinterpret_cast<char*>(v.data()), smd.end_pos);
}

/******************************************************************************/

//! append a POD to an ostream
template <typename T>
void stream_put_pod(std::ostream& os, const T& t) {
    static_assert(std::is_pod<T>::value, "T must be POD");
    os.write(reinterpret_cast<const char*>(&t), sizeof(T));
}

//! append a list of PODs to an ostream
static inline
void stream_put(std::ostream& /* os */) { }

//! append a list of PODs to an ostream
template <typename T, typename... Args>
void stream_put(std::ostream& os, const T& t, const Args& ... args) {
    stream_put_pod(os, t);
    stream_put(os, args...);
}

//! read a POD from an istream
template <typename T>
void stream_get_pod(std::istream& is, T& t) {
    static_assert(std::is_pod<T>::value, "T must be POD");
    is.read(reinterpret_cast<char*>(&t), sizeof(T));
}

//! read a list of PODs from an istream
static inline
void stream_get(std::istream& /* is */) { }

//! read a list of PODs from an istream
template <typename T, typename... Args>
void stream_get(std::istream& is, T& t, Args& ... args) {
    stream_get_pod(is, t);
    stream_get(is, args...);
}

} // namespace cobs

#endif // !COBS_UTIL_SERIALIZATION_HEADER

/******************************************************************************/
