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
const std::string DocumentHeader::file_extension = ".doc.isi";

void DocumentHeader::serialize(std::ofstream& ofs) const {
    stream_put(ofs, m_kmer_size);
    ofs << m_name << std::endl;
}

void DocumentHeader::deserialize(std::ifstream& ifs) {
    stream_get(ifs, m_kmer_size);
    std::getline(ifs, m_name);
}

DocumentHeader::DocumentHeader(std::string name, uint32_t kmer_size)
    : m_name(name), m_kmer_size(kmer_size) { }

std::string DocumentHeader::name() const {
    return m_name;
}

uint32_t DocumentHeader::kmer_size() const {
    return m_kmer_size;
}

} // namespace cobs

/******************************************************************************/
