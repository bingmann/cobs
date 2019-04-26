/*******************************************************************************
 * cobs/query/compact_index/mmap_search_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/compact_index/mmap_search_file.hpp>
#include <cobs/util/query.hpp>

#include <tlx/logger.hpp>
#include <tlx/math/div_ceil.hpp>

namespace cobs {

CompactIndexMMapSearchFile::CompactIndexMMapSearchFile(const fs::path& path)
    : CompactIndexSearchFile(path)
{
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

CompactIndexMMapSearchFile::~CompactIndexMMapSearchFile() {
    destroy_mmap(m_fd, m_data[0], stream_pos_);
}

void CompactIndexMMapSearchFile::read_from_disk(
    const std::vector<size_t>& hashes, char* rows,
    size_t begin, size_t size)
{
    size_t page_size = header_.page_size();

    die_unless(begin + size <= row_size());
    die_unless(begin % page_size == 0);
    size_t begin_page = begin / page_size;
    size_t end_page = tlx::div_ceil(begin + size, page_size);
    die_unless(end_page <= header_.parameters().size());

    LOG0 << "mmap::read_from_disk()"
         << " page_size=" << page_size
         << " hashes.size=" << hashes.size()
         << " begin=" << begin
         << " size=" << size
         << " begin_page=" << begin_page
         << " end_page=" << end_page;

    for (size_t i = 0; i < hashes.size(); i++) {
        size_t j = 0;
        for (size_t p = begin_page; p < end_page; ++p, ++j) {
            uint64_t hash = hashes[i] % header_.parameters()[p].signature_size;
            uint8_t* data_8 = m_data[p] + hash * page_size;
            uint8_t* rows_8 =
                reinterpret_cast<uint8_t*>(rows) + i * size + j * page_size;
            // die_unless(rows_8 + page_size <= rows + size * hashes.size());
            // std::memcpy(rows_8, data_8, page_size);
            std::copy(data_8, data_8 + page_size, rows_8);
        }
    }
}

} // namespace cobs

/******************************************************************************/
