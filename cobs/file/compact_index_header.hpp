/*******************************************************************************
 * cobs/file/compact_index_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_COMPACT_INDEX_HEADER_HEADER
#define COBS_FILE_COMPACT_INDEX_HEADER_HEADER

#include <cobs/file/header.hpp>

namespace cobs::file {

class compact_index_header : public header<compact_index_header>
{
public:
    struct parameter {
        uint64_t signature_size;
        uint64_t num_hashes;
    };

private:
    std::vector<parameter> m_parameters;
    std::vector<std::string> m_file_names;
    uint64_t m_page_size;
    size_t padding_size(uint64_t curr_stream_pos) const;

protected:
    void serialize(std::ofstream& ofs) const override;
    void deserialize(std::ifstream& ifs) override;

public:
    static const std::string magic_word;
    static const std::string file_extension;
    explicit compact_index_header(uint64_t page_size = 4096);
    compact_index_header(const std::vector<parameter>& parameters, const std::vector<std::string>& file_names, uint64_t page_size = 4096);
    const std::vector<parameter>& parameters() const;
    const std::vector<std::string>& file_names() const;
    uint64_t page_size() const;

    void read_file(std::ifstream& ifs, std::vector<std::vector<uint8_t> >& data);
    void read_file(const fs::path& p, std::vector<std::vector<uint8_t> >& data);
};

} // namespace cobs::file

#endif // !COBS_FILE_COMPACT_INDEX_HEADER_HEADER

/******************************************************************************/
