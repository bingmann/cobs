/*******************************************************************************
 * cobs/query/compact_index/aio_search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_COMPACT_INDEX_AIO_SEARCH_FILE_HEADER
#define COBS_QUERY_COMPACT_INDEX_AIO_SEARCH_FILE_HEADER

#include <cobs/query/compact_index/search_file.hpp>
#include <linux/aio_abi.h>

namespace cobs {

class CompactIndexAioSearchFile : public CompactIndexSearchFile
{
private:
    uint64_t m_max_nr_ios;
    int m_fd;
    aio_context_t m_ctx = 0;
    std::vector<uint64_t> m_offsets;
    std::vector<iocb> m_iocbs;
    std::vector<iocb*> m_iocbpp;
    std::vector<io_event> m_io_events;

protected:
    void read_from_disk(const std::vector<size_t>& hashes, uint8_t* rows,
                        size_t begin, size_t size, size_t buffer_size) override;

public:
    explicit CompactIndexAioSearchFile(const fs::path& path);
    ~CompactIndexAioSearchFile();
};

} // namespace cobs

#endif // !COBS_QUERY_COMPACT_INDEX_AIO_SEARCH_FILE_HEADER

/******************************************************************************/
