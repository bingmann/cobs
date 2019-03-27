/*******************************************************************************
 * cobs/file/kmer_buffer_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_KMER_BUFFER_HEADER_HEADER
#define COBS_FILE_KMER_BUFFER_HEADER_HEADER

#include <cobs/file/header.hpp>

namespace cobs {

class KMerBufferHeader
{
private:
    std::string m_name;
    uint32_t m_kmer_size;

public:
    static const std::string magic_word;
    static const uint32_t version;
    static const std::string file_extension;

    KMerBufferHeader() = default;
    KMerBufferHeader(std::string name, uint32_t kmer_size);

    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);

    std::string name() const;
    uint32_t kmer_size() const;
};

} // namespace cobs

#endif // !COBS_FILE_KMER_BUFFER_HEADER_HEADER

/******************************************************************************/
