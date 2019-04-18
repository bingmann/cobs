/*******************************************************************************
 * cobs/file/classic_index_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_CLASSIC_INDEX_HEADER_HEADER
#define COBS_FILE_CLASSIC_INDEX_HEADER_HEADER

#include <cobs/file/header.hpp>

namespace cobs {

class ClassicIndexHeader
{
private:
    uint32_t term_size_;
    uint8_t canonicalize_;
    uint64_t signature_size_;
    uint64_t num_hashes_;
    std::vector<std::string> file_names_;

public:
    static const std::string magic_word;
    static const uint32_t version;
    static const std::string file_extension;

    ClassicIndexHeader() = default;
    ClassicIndexHeader(
        uint32_t term_size, uint8_t canonicalize,
        uint64_t signature_size, uint64_t num_hashes,
        const std::vector<std::string>& file_names = std::vector<std::string>());

    uint32_t term_size() const;
    uint8_t canonicalize() const;
    uint64_t signature_size() const;
    uint64_t row_bits() const;
    uint64_t row_size() const;
    uint64_t num_hashes() const;
    const std::vector<std::string>& file_names() const;
    std::vector<std::string>& file_names();

    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);

    void write_file(std::ostream& os, const std::vector<uint8_t>& data);
    void write_file(const fs::path& p, const std::vector<uint8_t>& data);

    void read_file(std::istream& is, std::vector<uint8_t>& data);
    void read_file(const fs::path& p, std::vector<uint8_t>& data);
};

} // namespace cobs

#endif // !COBS_FILE_CLASSIC_INDEX_HEADER_HEADER

/******************************************************************************/
