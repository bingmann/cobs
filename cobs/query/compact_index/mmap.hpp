/*******************************************************************************
 * cobs/query/compact_index/mmap.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_COMPACT_INDEX_MMAP_HEADER
#define COBS_QUERY_COMPACT_INDEX_MMAP_HEADER

#include <cobs/query/compact_index/base.hpp>

namespace cobs::query::compact_index {

class mmap : public base
{
private:
    int m_fd;
    std::vector<uint8_t*> m_data;

protected:
    void read_from_disk(const std::vector<size_t>& hashes, char* rows) override;

public:
    explicit mmap(const fs::path& path);
    ~mmap();
};

} // namespace cobs::query::compact_index

#endif // !COBS_QUERY_COMPACT_INDEX_MMAP_HEADER

/******************************************************************************/
