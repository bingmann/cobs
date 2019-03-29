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

namespace cobs {

class CompactIndexHeader
{
public:
    struct parameter {
        uint64_t signature_size;
        uint64_t num_hashes;
    };

private:
    uint32_t term_size_;
    uint8_t canonicalize_;
    std::vector<parameter> parameters_;
    std::vector<std::string> file_names_;
    uint64_t page_size_;
    size_t padding_size(uint64_t curr_stream_pos) const;

public:
    static const std::string magic_word;
    static const uint32_t version;
    static const std::string file_extension;

    explicit CompactIndexHeader(uint64_t page_size = 4096);
    CompactIndexHeader(
        uint32_t term_size, uint8_t canonicalize,
        const std::vector<parameter>& parameters,
        const std::vector<std::string>& file_names, uint64_t page_size = 4096);

    uint32_t term_size() const;
    uint8_t canonicalize() const;
    const std::vector<parameter>& parameters() const;
    const std::vector<std::string>& file_names() const;
    uint64_t page_size() const;

    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);

    void read_file(std::istream& is, std::vector<std::vector<uint8_t> >& data);
    void read_file(const fs::path& p, std::vector<std::vector<uint8_t> >& data);
};

} // namespace cobs

#endif // !COBS_FILE_COMPACT_INDEX_HEADER_HEADER

/******************************************************************************/
