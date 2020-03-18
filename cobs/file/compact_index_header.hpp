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

    //! k-mer or q-gram size = term size
    uint32_t term_size_;
    //! 0 = don't modify k-mers, 1 = canonicalize
    uint8_t canonicalize_;
    //! parameters of subindices
    std::vector<parameter> parameters_;
    //! list of document file names
    std::vector<std::string> file_names_;
    //! size of each subindex in bytes
    uint64_t page_size_;

    size_t padding_size(uint64_t curr_stream_pos) const;

public:
    static const std::string magic_word;
    static const uint32_t version;
    static const std::string file_extension;

    explicit CompactIndexHeader(uint64_t page_size = 4096);

    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);

    void read_file(std::istream& is, std::vector<std::vector<uint8_t> >& data);
    void read_file(const fs::path& p, std::vector<std::vector<uint8_t> >& data);
};

} // namespace cobs

#endif // !COBS_FILE_COMPACT_INDEX_HEADER_HEADER

/******************************************************************************/
