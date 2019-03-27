/*******************************************************************************
 * cobs/file/kmer_buffer_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/kmer_buffer_header.hpp>
#include <cobs/util/file.hpp>

namespace cobs {

const std::string KMerBufferHeader::magic_word = "DOCUMENT";
const uint32_t KMerBufferHeader::version = 1;
const std::string KMerBufferHeader::file_extension = ".cobs_doc";

KMerBufferHeader::KMerBufferHeader(std::string name, uint32_t kmer_size)
    : m_name(name), m_kmer_size(kmer_size) { }

void KMerBufferHeader::serialize(std::ostream& os) const {
    serialize_magic_begin(os, magic_word, version);

    stream_put(os, m_kmer_size);
    os << m_name << '\0';

    serialize_magic_end(os, magic_word);
}

void KMerBufferHeader::deserialize(std::istream& is) {
    deserialize_magic_begin(is, magic_word, version);

    stream_get(is, m_kmer_size);
    std::getline(is, m_name, '\0');

    deserialize_magic_end(is, magic_word);
}

std::string KMerBufferHeader::name() const {
    return m_name;
}

uint32_t KMerBufferHeader::kmer_size() const {
    return m_kmer_size;
}

} // namespace cobs

/******************************************************************************/
