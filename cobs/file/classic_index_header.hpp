/*******************************************************************************
 * cobs/file/classic_index_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_CLASSIC_INDEX_HEADER_HEADER
#define COBS_FILE_CLASSIC_INDEX_HEADER_HEADER
#pragma once

#include <cobs/file/header.hpp>

namespace cobs::file {

class classic_index_header : public header<classic_index_header>
{
private:
    uint64_t m_signature_size;
    uint64_t m_num_hashes;
    std::vector<std::string> m_file_names;

protected:
    void serialize(std::ofstream& ofs) const override;
    void deserialize(std::ifstream& ifs) override;

public:
    static const std::string magic_word;
    static const std::string file_extension;
    classic_index_header() = default;
    classic_index_header(
        uint64_t signature_size, uint64_t num_hashes,
        const std::vector<std::string>& file_names = std::vector<std::string>());
    uint64_t signature_size() const;
    uint64_t block_size() const;
    uint64_t num_hashes() const;
    const std::vector<std::string>& file_names() const;
    std::vector<std::string>& file_names();

    void write_file(std::ofstream& ofs, const std::vector<uint8_t>& data);
    void write_file(const fs::path& p, const std::vector<uint8_t>& data);

    void read_file(std::ifstream& ifs, std::vector<uint8_t>& data);
    void read_file(const fs::path& p, std::vector<uint8_t>& data);
};

} // namespace cobs::file

#endif // !COBS_FILE_CLASSIC_INDEX_HEADER_HEADER

/******************************************************************************/
