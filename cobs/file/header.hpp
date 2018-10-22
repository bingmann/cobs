/*******************************************************************************
 * cobs/file/header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_HEADER_HEADER
#define COBS_FILE_HEADER_HEADER
#pragma once

#include <ostream>
#include <string>
#include <vector>

#include <cobs/file/file_io_exception.hpp>
#include <cobs/util/error_handling.hpp>
#include <cobs/util/serialization.hpp>

namespace cobs::file {

template <class T>
class header
{
private:
    static const uint32_t m_version;

protected:
    virtual void serialize(std::ofstream& ofs) const = 0;
    virtual void deserialize(std::ifstream& ifs) = 0;

public:
    static const std::string magic_word;
    static void serialize(std::ofstream& ofs, const header& h);
    static void deserialize(std::ifstream& ifs, header& h);
};

template <class T>
const std::string header<T>::magic_word = "INSIIN";
template <class T>
const uint32_t header<T>::m_version = 1;

static inline
void check_magic_word(std::ifstream& ifs, const std::string& magic_word) {
    std::vector<char> mw_v(magic_word.size(), ' ');
    ifs.read(mw_v.data(), magic_word.size());
    std::string mw(mw_v.data(), mw_v.size());
    assert_throw<file_io_exception>(mw == magic_word, "invalid file type");
    assert_throw<file_io_exception>(ifs.good(), "input filestream broken");
}

template <class T>
void header<T>::serialize(std::ofstream& ofs, const header& h) {
    ofs << magic_word;
    cobs::serialize(ofs, m_version);
    h.serialize(ofs);
    ofs << T::magic_word;
}

template <class T>
void header<T>::deserialize(std::ifstream& ifs, header& h) {
    check_magic_word(ifs, magic_word);
    uint32_t v;
    cobs::deserialize(ifs, v);
    assert_throw<file_io_exception>(v == m_version, "invalid file version");
    h.deserialize(ifs);
    check_magic_word(ifs, T::magic_word);
}

} // namespace cobs::file

#endif // !COBS_FILE_HEADER_HEADER

/******************************************************************************/
