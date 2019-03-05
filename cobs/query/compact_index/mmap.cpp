/*******************************************************************************
 * cobs/query/compact_index/mmap.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/compact_index/mmap.hpp>
#include <cobs/util/query.hpp>

namespace cobs::query::compact_index {

mmap::mmap(const fs::path& path) : compact_index::base(path) {
    m_data.resize(m_header.parameters().size());
    std::pair<int, uint8_t*> handles = initialize_mmap(path, stream_pos_);
    m_fd = handles.first;
    m_data[0] = handles.second;
    for (size_t i = 1; i < m_header.parameters().size(); i++) {
        m_data[i] = m_data[i - 1] + m_header.page_size() * m_header.parameters()[i - 1].signature_size;
    }
}

mmap::~mmap() {
    destroy_mmap(m_fd, m_data[0], stream_pos_);
}

void mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for collapse(2)
    for (size_t i = 0; i < m_header.parameters().size(); i++) {
        for (size_t j = 0; j < hashes.size(); j++) {
            // todo this is wrong, two times modulo results in a different line?!
            // works because max uint should still change since confusing
            uint64_t hash = hashes[j] % m_header.parameters()[i].signature_size;
            auto data_8 = m_data[i] + hash * m_header.page_size();
            auto rows_8 = rows + (i + j * m_header.parameters().size()) * m_header.page_size();
            std::memcpy(rows_8, data_8, m_header.page_size());
        }
    }
}

} // namespace cobs::query::compact_index

/******************************************************************************/
