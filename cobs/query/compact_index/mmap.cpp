/*******************************************************************************
 * cobs/query/compact_index/mmap.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/compact_index/mmap.hpp>
#include <cobs/util/query.hpp>

#include <tlx/math/div_ceil.hpp>

namespace cobs::query::compact_index {

mmap::mmap(const fs::path& path) : compact_index::base(path) {
    m_data.resize(header_.parameters().size());
    std::pair<int, uint8_t*> handles = initialize_mmap(path, stream_pos_);
    m_fd = handles.first;
    m_data[0] = handles.second;
    for (size_t i = 1; i < header_.parameters().size(); i++) {
        m_data[i] =
            m_data[i - 1]
            + header_.page_size() * header_.parameters()[i - 1].signature_size;
    }
}

mmap::~mmap() {
    destroy_mmap(m_fd, m_data[0], stream_pos_);
}

void mmap::read_from_disk(const std::vector<size_t>& hashes, uint8_t* rows,
                          size_t begin, size_t size) {
    die_unless(begin + size <= row_size());
    die_unless(begin % header_.page_size() == 0);
    size_t begin_page = begin / header_.page_size();
    size_t end_page = tlx::div_ceil(begin + size, header_.page_size());
    die_unless(end_page <= header_.parameters().size());

    for (size_t i = 0; i < hashes.size(); i++) {
        size_t j = 0;
        for (size_t p = begin_page; p < end_page; ++p, ++j) {
            uint64_t hash = hashes[i] % header_.parameters()[p].signature_size;
            auto data_8 = m_data[p] + hash * header_.page_size();
            auto rows_8 = rows + j * header_.page_size() + i * size;
            // die_unless(rows_8 + header_.page_size() <= rows + size * hashes.size());
            std::memcpy(rows_8, data_8, header_.page_size());
        }
    }
}

} // namespace cobs::query::compact_index

/******************************************************************************/
