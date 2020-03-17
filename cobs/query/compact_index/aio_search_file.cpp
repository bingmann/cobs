/*******************************************************************************
 * cobs/query/compact_index/aio_search_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/compact_index/aio_search_file.hpp>
#include <cobs/util/aio.hpp>
#include <cobs/util/error_handling.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/misc.hpp>
#include <cobs/util/query.hpp>

#include <cstring>
#include <fcntl.h>

#include <tlx/unused.hpp>

namespace cobs {

CompactIndexAioSearchFile::CompactIndexAioSearchFile(const fs::path& path)
    : CompactIndexSearchFile(path), m_max_nr_ios(65536 * header_.parameters_.size()),
      m_iocbs(m_max_nr_ios), m_iocbpp(m_max_nr_ios), m_io_events(m_max_nr_ios)
{
    // todo use sysctl to check max-nr-io
    assert_exit(header_.page_size_ % cobs::get_page_size() == 0,
                "page size needs to be divisible by 4096 "
                "so the index can be opened with O_DIRECT");

    m_offsets.resize(header_.parameters_.size());
    m_offsets[0] = stream_pos_.curr_pos;
    for (size_t i = 1; i < header_.parameters_.size(); i++) {
        m_offsets[i] = m_offsets[i - 1] + header_.page_size_ * header_.parameters_[i - 1].signature_size;
    }

    m_fd = open_file(path, O_RDONLY | O_DIRECT);
    if (io_setup(m_max_nr_ios, &m_ctx) < 0) {
        exit_error_errno("io_setup error");
    }

    for (size_t i = 0; i < m_iocbs.size(); i++) {
        m_iocbs[i].aio_fildes = m_fd;
        m_iocbs[i].aio_lio_opcode = IOCB_CMD_PREAD;
        m_iocbs[i].aio_nbytes = header_.page_size_;
        m_iocbpp[i] = m_iocbs.data() + i;
    }
}

CompactIndexAioSearchFile::~CompactIndexAioSearchFile() {
    close_file(m_fd);
    if (io_destroy(m_ctx) < 0) {
        exit_error_errno("io_destroy error");
    }
}

void CompactIndexAioSearchFile::read_from_disk(
    const std::vector<size_t>& hashes, uint8_t* rows,
    size_t begin, size_t size, size_t buffer_size)
{
    tlx::unused(begin, size, buffer_size);

    int64_t num_requests = header_.parameters_.size() * hashes.size();

#pragma omp parallel for collapse(2)
    for (size_t i = 0; i < header_.parameters_.size(); i++) {
        for (size_t j = 0; j < hashes.size(); j++) {
            uint64_t index = i + j * header_.parameters_.size();
            uint64_t hash = hashes[j] % header_.parameters_[i].signature_size;
            // todo rows does not need to be reallocated each time
            m_iocbs[index].aio_buf = (uint64_t)rows + index * header_.page_size_;
            m_iocbs[index].aio_offset = m_offsets[i] + hash * header_.page_size_;
        }
    }

    int ret = io_submit(m_ctx, num_requests, m_iocbpp.data());
    if (ret != num_requests) {
        if (ret < 0) {
            perror("io_submit error");
        }
        else {
            fprintf(stderr, "could not sumbit IOs");
        }
        exit_error_errno("io_submit error");
    }

    if (io_getevents(m_ctx, num_requests, num_requests, m_io_events.data(), nullptr) < num_requests) {
        exit_error_errno("io_getevents error");
    }

    for (int i = 0; i < num_requests; i++) {
        if (m_io_events[i].res != (int64_t)header_.page_size_) {
            std::cout << i << " " << std::strerror(-m_io_events[i].res) << std::endl;
        }
    }
}

} // namespace cobs

/******************************************************************************/
