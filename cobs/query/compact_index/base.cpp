/*******************************************************************************
 * cobs/query/compact_index/base.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/compact_index/base.hpp>

#include <cobs/util/file.hpp>

#include <tlx/die.hpp>

namespace cobs::query::compact_index {

base::base(const fs::path& path) : query::classic_base() {
    std::ifstream ifs;
    m_header = deserialize_header<CompactIndexHeader>(ifs, path);
    stream_pos_ = get_stream_pos(ifs);

    // todo assertions that all the data in the Header is correct
    m_row_size = m_header.page_size() * m_header.parameters().size();
    m_num_hashes = m_header.parameters()[0].num_hashes;
    for (const auto& p : m_header.parameters()) {
        die_unless(m_num_hashes == p.num_hashes);
    }
}

uint64_t base::num_hashes() const {
    return m_num_hashes;
}

uint64_t base::row_size() const {
    return m_row_size;
}

uint64_t base::counts_size() const {
    return 8 * m_header.parameters().size() * m_header.page_size();
}

const std::vector<std::string>& base::file_names() const {
    return m_header.file_names();
}

} // namespace cobs::query::compact_index

/******************************************************************************/
