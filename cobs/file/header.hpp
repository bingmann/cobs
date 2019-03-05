/*******************************************************************************
 * cobs/file/header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_HEADER_HEADER
#define COBS_FILE_HEADER_HEADER

#include <ostream>
#include <string>
#include <vector>

#include <cobs/file/file_io_exception.hpp>
#include <cobs/util/error_handling.hpp>
#include <cobs/util/serialization.hpp>

namespace cobs {

static inline
void check_magic_word(std::istream& is, const std::string& magic_word) {
    std::vector<char> mw_v(magic_word.size(), ' ');
    is.read(mw_v.data(), magic_word.size());
    std::string mw(mw_v.data(), mw_v.size());
    assert_throw<FileIOException>(mw == magic_word, "invalid file type");
    assert_throw<FileIOException>(is.good(), "input filestream broken");
}

static inline
void serialize_magic_begin(
    std::ostream& os, const std::string& magic_word, const uint32_t& version) {
    os << "COBS:";
    os << magic_word;
    stream_put(os, version);
}

static inline
void serialize_magic_end(
    std::ostream& os, const std::string& magic_word) {
    os << magic_word;
}

static inline
void deserialize_magic_begin(
    std::istream& is, const std::string& magic_word, const uint32_t& version) {
    check_magic_word(is, "COBS:");
    check_magic_word(is, magic_word);
    uint32_t v;
    stream_get(is, v);
    assert_throw<FileIOException>(v == version, "invalid file version");
}

static inline
void deserialize_magic_end(
    std::istream& is, const std::string& magic_word) {
    check_magic_word(is, magic_word);
}

} // namespace cobs

#endif // !COBS_FILE_HEADER_HEADER

/******************************************************************************/
