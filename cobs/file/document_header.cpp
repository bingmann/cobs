/*******************************************************************************
 * cobs/file/document_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/document_header.hpp>
#include <cobs/util/file.hpp>

namespace cobs {

const std::string DocumentHeader::magic_word = "DOCUMENT";
const uint32_t DocumentHeader::version = 1;
const std::string DocumentHeader::file_extension = ".cobs_doc";

DocumentHeader::DocumentHeader(std::string name, uint32_t kmer_size)
    : m_name(name), m_kmer_size(kmer_size) { }

void DocumentHeader::serialize(std::ostream& os) const {
    serialize_magic_begin(os, magic_word, version);

    stream_put(os, m_kmer_size);
    os << m_name << '\0';

    serialize_magic_end(os, magic_word);
}

void DocumentHeader::deserialize(std::istream& is) {
    deserialize_magic_begin(is, magic_word, version);

    stream_get(is, m_kmer_size);
    std::getline(is, m_name, '\0');

    deserialize_magic_end(is, magic_word);
}

std::string DocumentHeader::name() const {
    return m_name;
}

uint32_t DocumentHeader::kmer_size() const {
    return m_kmer_size;
}

} // namespace cobs

/******************************************************************************/
