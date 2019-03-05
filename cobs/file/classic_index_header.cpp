/*******************************************************************************
 * cobs/file/classic_index_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/classic_index_header.hpp>
#include <cobs/util/file.hpp>

namespace cobs {

const std::string ClassicIndexHeader::magic_word = "CLASSIC_INDEX";
const uint32_t ClassicIndexHeader::version = 1;
const std::string ClassicIndexHeader::file_extension = ".cla_idx.cobs";

ClassicIndexHeader::ClassicIndexHeader(uint64_t signature_size, uint64_t num_hashes, const std::vector<std::string>& file_names)
    : m_signature_size(signature_size), m_num_hashes(num_hashes), m_file_names(file_names) { }

uint64_t ClassicIndexHeader::signature_size() const {
    return m_signature_size;
}

uint64_t ClassicIndexHeader::block_size() const {
    return (m_file_names.size() + 7) / 8;
}

uint64_t ClassicIndexHeader::num_hashes() const {
    return m_num_hashes;
}

const std::vector<std::string>& ClassicIndexHeader::file_names() const {
    return m_file_names;
}

std::vector<std::string>& ClassicIndexHeader::file_names() {
    return m_file_names;
}

void ClassicIndexHeader::serialize(std::ostream& os) const {
    serialize_magic_begin(os, magic_word, version);

    stream_put(os, (uint32_t)m_file_names.size(), m_signature_size, m_num_hashes);
    for (const auto& file_name : m_file_names) {
        os << file_name << std::endl;
    }

    serialize_magic_end(os, magic_word);
}

void ClassicIndexHeader::deserialize(std::istream& is) {
    deserialize_magic_begin(is, magic_word, version);

    uint32_t file_names_size;
    stream_get(is, file_names_size, m_signature_size, m_num_hashes);
    m_file_names.resize(file_names_size);
    for (auto& file_name : m_file_names) {
        std::getline(is, file_name);
    }

    deserialize_magic_end(is, magic_word);
}

void ClassicIndexHeader::write_file(std::ostream& os,
                                    const std::vector<uint8_t>& data) {
    os.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    serialize(os);
    os.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ClassicIndexHeader::write_file(const fs::path& p,
                                    const std::vector<uint8_t>& data) {
    if (!p.parent_path().empty())
        fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
    write_file(ofs, data);
}

void ClassicIndexHeader::read_file(std::istream& is,
                                   std::vector<uint8_t>& data) {
    is.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    deserialize(is);
    size_t size = get_stream_size(is);
    data.resize(size);
    is.read(reinterpret_cast<char*>(data.data()), size);
}

void ClassicIndexHeader::read_file(const fs::path& p,
                                   std::vector<uint8_t>& data) {
    std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
    read_file(ifs, data);
}

} // namespace cobs

/******************************************************************************/
