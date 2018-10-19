/*******************************************************************************
 * cobs/file/document_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/document_header.hpp>
#include <cobs/util/file.hpp>

namespace cobs::file {

const std::string document_header::magic_word = "DOCUMENT";
const std::string document_header::file_extension = ".doc.isi";

void document_header::serialize(std::ofstream& ofs) const {
    cobs::serialize(ofs, m_kmer_size);
    ofs << m_name << std::endl;
}

void document_header::deserialize(std::ifstream& ifs) {
    cobs::deserialize(ifs, m_kmer_size);
    std::getline(ifs, m_name);
}

document_header::document_header(std::string name, uint32_t kmer_size)
    : m_name(name), m_kmer_size(kmer_size) { }

std::string document_header::name() const {
    return m_name;
}

uint32_t document_header::kmer_size() const {
    return m_kmer_size;
}

} // namespace cobs::file

/******************************************************************************/
