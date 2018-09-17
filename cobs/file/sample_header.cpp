/*******************************************************************************
 * cobs/file/sample_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/sample_header.hpp>
#include <cobs/util/file.hpp>

namespace cobs::file {

const std::string sample_header::magic_word = "SAMPLE";
const std::string sample_header::file_extension = ".sam.isi";

void sample_header::serialize(std::ofstream& ofs) const {
    cobs::serialize(ofs, m_kmer_size);
    ofs << m_name << std::endl;
}

void sample_header::deserialize(std::ifstream& ifs) {
    cobs::deserialize(ifs, m_kmer_size);
    std::getline(ifs, m_name);
}

sample_header::sample_header(std::string name, uint32_t kmer_size) : m_name(name), m_kmer_size(kmer_size) { }

std::string sample_header::name() const {
    return m_name;
}

uint32_t sample_header::kmer_size() const {
    return m_kmer_size;
}

} // namespace cobs::file

/******************************************************************************/
