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
public:
    //! k-mer or q-gram size = term size
    uint32_t term_size_;
    //! 0 = don't modify k-mers, 1 = canonicalize
    uint8_t canonicalize_;
    //! size of Bloom filters in bits = number of rows of matrix
    uint64_t signature_size_;
    //! number of hashes per term, usually 1
    uint64_t num_hashes_;
    //! list of document file names
    std::vector<std::string> file_names_;

public:
    static const std::string magic_word;
    static const uint32_t version;
    static const std::string file_extension;

    ClassicIndexHeader() = default;

    //! number of bits in a row, which is the number of documents
    uint64_t row_bits() const;
    //! number of bytes in a row, number of documents rounded up to bytes.
    uint64_t row_size() const;

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
