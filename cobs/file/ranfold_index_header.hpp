/*******************************************************************************
 * cobs/file/ranfold_index_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_RANFOLD_INDEX_HEADER_HEADER
#define COBS_FILE_RANFOLD_INDEX_HEADER_HEADER
#pragma once

#include <cobs/file/header.hpp>

namespace cobs::file {

class ranfold_index_header : public header<ranfold_index_header>
{
public:
    uint64_t m_term_space;
    uint32_t m_term_hashes;
    uint64_t m_doc_space_bytes;
    uint32_t m_doc_hashes;
    std::vector<std::string> m_file_names;
    std::vector<uint32_t> m_doc_array;

    static const uint32_t mark = 0x80000000;
    static const uint32_t mark_mask = ~mark;

protected:
    void serialize(std::ofstream& ofs) const override;
    void deserialize(std::ifstream& ifs) override;

public:
    static const std::string magic_word;
    static const std::string file_extension;
    ranfold_index_header() = default;

    void write_file(std::ofstream& ofs, const std::vector<uint8_t>& data);
    void write_file(const fs::path& p, const std::vector<uint8_t>& data);

    void read_file(std::ifstream& ifs, std::vector<uint8_t>& data);
    void read_file(const fs::path& p, std::vector<uint8_t>& data);
};

} // namespace cobs::file

#endif // !COBS_FILE_RANFOLD_INDEX_HEADER_HEADER

/******************************************************************************/
