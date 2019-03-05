/*******************************************************************************
 * cobs/file/ranfold_index_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/ranfold_index_header.hpp>
#include <cobs/util/file.hpp>

namespace cobs {

const std::string RanfoldIndexHeader::magic_word = "RANFOLD_INDEX";
const uint32_t RanfoldIndexHeader::version = 1;
const std::string RanfoldIndexHeader::file_extension = ".rfd_idx.isi";

void RanfoldIndexHeader::serialize(std::ofstream& ofs) const {
    serialize_magic_begin(ofs, magic_word, version);

    stream_put(ofs,
               m_term_space, m_term_hashes,
               m_doc_space_bytes, m_doc_hashes,
               (uint32_t)m_file_names.size(), (uint32_t)m_doc_array.size());
    for (const auto& file_name : m_file_names) {
        ofs << file_name << std::endl;
    }
    // for (const auto& d : m_doc_array) {
    //     cobs::serialize(ofs, d);
    // }

    serialize_magic_begin(ofs, magic_word, version);
}

void RanfoldIndexHeader::deserialize(std::ifstream& ifs) {
    deserialize_magic_begin(ifs, magic_word, version);

    uint32_t file_names_size, doc_array_size;
    stream_get(ifs,
               m_term_space, m_term_hashes,
               m_doc_space_bytes, m_doc_hashes,
               file_names_size, doc_array_size);
    m_file_names.resize(file_names_size);
    for (auto& file_name : m_file_names) {
        std::getline(ifs, file_name);
    }
    m_doc_array.resize(doc_array_size);
    for (auto& d : m_doc_array) {
        stream_get(ifs, d);
    }

    deserialize_magic_end(ifs, magic_word);
}

void RanfoldIndexHeader::write_file(std::ofstream& ofs,
                                    const std::vector<uint8_t>& data) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    serialize(ofs);
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void RanfoldIndexHeader::write_file(const fs::path& p,
                                    const std::vector<uint8_t>& data) {
    if (!p.parent_path().empty())
        fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
    write_file(ofs, data);
}

void RanfoldIndexHeader::read_file(std::ifstream& ifs,
                                   std::vector<uint8_t>& data) {
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    deserialize(ifs);
    size_t size = get_stream_size(ifs);
    data.resize(size);
    ifs.read(reinterpret_cast<char*>(data.data()), size);
}

void RanfoldIndexHeader::read_file(const fs::path& p,
                                   std::vector<uint8_t>& data) {
    std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
    read_file(ifs, data);
}

} // namespace cobs

/******************************************************************************/
