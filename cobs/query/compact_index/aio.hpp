/*******************************************************************************
 * cobs/query/compact_index/aio.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_COMPACT_INDEX_AIO_HEADER
#define COBS_QUERY_COMPACT_INDEX_AIO_HEADER

#include <cobs/query/compact_index/base.hpp>
#include <linux/aio_abi.h>

namespace cobs::query::compact_index {

class aio : public base
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
    void read_from_disk(const std::vector<size_t>& hashes, char* rows,
                        size_t begin, size_t size) override;

public:
    explicit aio(const fs::path& path);
    ~aio();
};

} // namespace cobs::query::compact_index

#endif // !COBS_QUERY_COMPACT_INDEX_AIO_HEADER

/******************************************************************************/
