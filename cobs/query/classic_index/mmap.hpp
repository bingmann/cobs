/*******************************************************************************
 * cobs/query/classic_index/mmap.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_INDEX_MMAP_HEADER
#define COBS_QUERY_CLASSIC_INDEX_MMAP_HEADER

#include <cobs/query/classic_index/base.hpp>

namespace cobs::query::classic_index {

class mmap : public base
{
private:
    int m_fd;
    uint8_t* m_data;

protected:
    void read_from_disk(const std::vector<size_t>& hashes, char* rows,
                        size_t begin, size_t size) override;

public:
    explicit mmap(const fs::path& path);
    ~mmap();
};

} // namespace cobs::query::classic_index

#endif // !COBS_QUERY_CLASSIC_INDEX_MMAP_HEADER

/******************************************************************************/
