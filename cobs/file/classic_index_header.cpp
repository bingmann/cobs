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
const std::string ClassicIndexHeader::file_extension = ".cla_idx.isi";

void ClassicIndexHeader::serialize(std::ofstream& ofs) const {
    cobs::serialize(ofs, (uint32_t)m_file_names.size(), m_signature_size, m_num_hashes);
    for (const auto& file_name : m_file_names) {
        ofs << file_name << std::endl;
    }
}

void ClassicIndexHeader::deserialize(std::ifstream& ifs) {
    uint32_t file_names_size;
    cobs::deserialize(ifs, file_names_size, m_signature_size, m_num_hashes);
    m_file_names.resize(file_names_size);
    for (auto& file_name : m_file_names) {
        std::getline(ifs, file_name);
    }
}

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

void ClassicIndexHeader::write_file(std::ofstream& ofs,
                                    const std::vector<uint8_t>& data) {
    ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    Header<ClassicIndexHeader>::serialize(ofs, *this);
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ClassicIndexHeader::write_file(const fs::path& p,
                                    const std::vector<uint8_t>& data) {
    if (!p.parent_path().empty())
        fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
    write_file(ofs, data);
}

void ClassicIndexHeader::read_file(std::ifstream& ifs,
                                   std::vector<uint8_t>& data) {
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    Header<ClassicIndexHeader>::deserialize(ifs, *this);
    stream_metadata smd = get_stream_metadata(ifs);
    size_t size = smd.end_pos - smd.curr_pos;
    data.resize(size);
    ifs.read(reinterpret_cast<char*>(data.data()), size);
}

void ClassicIndexHeader::read_file(const fs::path& p,
                                   std::vector<uint8_t>& data) {
    std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
    read_file(ifs, data);
}

} // namespace cobs

/******************************************************************************/
