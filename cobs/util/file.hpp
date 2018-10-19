/*******************************************************************************
 * cobs/util/file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_FILE_HEADER
#define COBS_UTIL_FILE_HEADER
#pragma once

#include <algorithm>
#include <iostream>

#include <cobs/document.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/document_header.hpp>
#include <cobs/file/frequency_header.hpp>
#include <cobs/util/fs.hpp>

namespace cobs::file {

static document_header dummy_sh;
static classic_index_header dummy_isih;
static compact_index_header dummy_cisih;

template <uint32_t N>
void serialize(std::ofstream& ofs, const document<N>& s, const std::string& name);

template <uint32_t N>
void serialize(const fs::path& p, const document<N>& s, const std::string& name);

void serialize(std::ofstream& ofs, const std::vector<uint8_t>& data, const classic_index_header& h);
void serialize(const fs::path& p, const std::vector<uint8_t>& data, const classic_index_header& h);

template <uint32_t N>
void deserialize(std::ifstream& ifs, document<N>& s, document_header& h = dummy_sh);

template <uint32_t N>
void deserialize(const fs::path& p, document<N>& s, document_header& h = dummy_sh);

void deserialize(std::ifstream& ifs, std::vector<uint8_t>& data, classic_index_header& h = dummy_isih);
void deserialize(std::ifstream& ifs, std::vector<std::vector<uint8_t> >& data, compact_index_header& h = dummy_cisih);
void deserialize(const fs::path& p, std::vector<uint8_t>& data, classic_index_header& h = dummy_isih);
void deserialize(const fs::path& p, std::vector<std::vector<uint8_t> >& data, compact_index_header& h = dummy_cisih);

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

template <uint32_t N>
void serialize(std::ofstream& ofs, const document<N>& s, const std::string& name) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    document_header sh(name, N);
    header<document_header>::serialize(ofs, sh);
    ofs.write(reinterpret_cast<const char*>(s.data().data()), kmer<N>::size* s.data().size());
}

template <uint32_t N>
void serialize(const fs::path& p, const document<N>& s, const std::string& name) {
    fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
    serialize(ofs, s, name);
}

template <uint32_t N>
void deserialize(std::ifstream& ifs, document<N>& s, document_header& h) {
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    header<document_header>::deserialize(ifs, h);
    assert(N == h.kmer_size());

    stream_metadata smd = get_stream_metadata(ifs);
    size_t size = smd.end_pos - smd.curr_pos;
    s.data().resize(size / kmer<N>::size);
    ifs.read(reinterpret_cast<char*>(s.data().data()), size);
}

template <uint32_t N>
void deserialize(const fs::path& p, document<N>& s, document_header& h) {
    std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
    deserialize(ifs, s, h);
}

template <class T>
void serialize_header(std::ofstream& ofs, const fs::path& p, const T& h) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    ofs.open(p.string(), std::ios::out | std::ios::binary);
    header<T>::serialize(ofs, h);
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
    header<T>::deserialize(ifs, h);
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
