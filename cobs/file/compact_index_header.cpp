/*******************************************************************************
 * cobs/file/compact_index_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/compact_index_header.hpp>

namespace cobs {

const std::string CompactIndexHeader::magic_word = "COMPACT_INDEX";
const uint32_t CompactIndexHeader::version = 1;
const std::string CompactIndexHeader::file_extension = ".com_idx.isi";

CompactIndexHeader::CompactIndexHeader(uint64_t page_size) : m_page_size(page_size) { }

CompactIndexHeader::CompactIndexHeader(const std::vector<CompactIndexHeader::parameter>& parameters, const std::vector<std::string>& file_names, uint64_t page_size)
    : m_parameters(parameters), m_file_names(file_names), m_page_size(page_size) { }

size_t CompactIndexHeader::padding_size(uint64_t curr_stream_pos) const {
    return (m_page_size - ((curr_stream_pos + CompactIndexHeader::magic_word.size()) % m_page_size)) % m_page_size;
}

void CompactIndexHeader::serialize(std::ostream& os) const {
    serialize_magic_begin(os, magic_word, version);

    stream_put(os, (uint32_t)m_parameters.size(), (uint32_t)m_file_names.size(), m_page_size);
    os.flush();
    for (const auto& p : m_parameters) {
        cobs::stream_put(os, p.signature_size, p.num_hashes);
    }
    for (const auto& file_name : m_file_names) {
        os << file_name << std::endl;
    }

    std::vector<char> padding(padding_size(os.tellp()));
    os.write(padding.data(), padding.size());

    serialize_magic_end(os, magic_word);
}

void CompactIndexHeader::deserialize(std::istream& is) {
    deserialize_magic_begin(is, magic_word, version);

    uint32_t parameters_size;
    uint32_t file_names_size;
    stream_get(is, parameters_size, file_names_size, m_page_size);
    m_parameters.resize(parameters_size);
    for (auto& p : m_parameters) {
        stream_get(is, p.signature_size, p.num_hashes);
    }

    m_file_names.resize(file_names_size);
    for (auto& file_name : m_file_names) {
        std::getline(is, file_name);
    }

    StreamPos sp = get_stream_pos(is);
    is.seekg(sp.curr_pos + padding_size(sp.curr_pos), std::ios::beg);

    deserialize_magic_end(is, magic_word);
}

const std::vector<CompactIndexHeader::parameter>& CompactIndexHeader::parameters() const {
    return m_parameters;
}

const std::vector<std::string>& CompactIndexHeader::file_names() const {
    return m_file_names;
}

uint64_t CompactIndexHeader::page_size() const {
    return m_page_size;
}

void CompactIndexHeader::read_file(std::istream& is,
                                   std::vector<std::vector<uint8_t> >& data) {
    is.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    deserialize(is);
    data.clear();
    data.resize(parameters().size());
    for (size_t i = 0; i < parameters().size(); i++) {
        size_t data_size = page_size() * parameters()[i].signature_size;
        std::vector<uint8_t> d(data_size);
        is.read(reinterpret_cast<char*>(d.data()), data_size);
        data[i] = std::move(d);
    }
}

void CompactIndexHeader::read_file(const fs::path& p,
                                   std::vector<std::vector<uint8_t> >& data) {
    std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
    read_file(ifs, data);
}

} // namespace cobs

/******************************************************************************/
