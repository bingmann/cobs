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

template <class T>
class Header
{
private:
    static const uint32_t m_version;

protected:
    virtual void serialize(std::ofstream& ofs) const = 0;
    virtual void deserialize(std::ifstream& ifs) = 0;

public:
    static const std::string magic_word;
    static void serialize(std::ofstream& ofs, const Header& h);
    static void deserialize(std::ifstream& ifs, Header& h);
};

template <class T>
const std::string Header<T>::magic_word = "INSIIN";
template <class T>
const uint32_t Header<T>::m_version = 1;

static inline
void check_magic_word(std::ifstream& ifs, const std::string& magic_word) {
    std::vector<char> mw_v(magic_word.size(), ' ');
    ifs.read(mw_v.data(), magic_word.size());
    std::string mw(mw_v.data(), mw_v.size());
    assert_throw<FileIOException>(mw == magic_word, "invalid file type");
    assert_throw<FileIOException>(ifs.good(), "input filestream broken");
}

template <class T>
void Header<T>::serialize(std::ofstream& ofs, const Header& h) {
    ofs << magic_word;
    stream_put(ofs, m_version);
    h.serialize(ofs);
    ofs << T::magic_word;
}

template <class T>
void Header<T>::deserialize(std::ifstream& ifs, Header& h) {
    check_magic_word(ifs, magic_word);
    uint32_t v;
    stream_get(ifs, v);
    assert_throw<FileIOException>(v == m_version, "invalid file version");
    h.deserialize(ifs);
    check_magic_word(ifs, T::magic_word);
}

} // namespace cobs

#endif // !COBS_FILE_HEADER_HEADER

/******************************************************************************/
