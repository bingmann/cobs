#include <isi/query/compact_index/aio.hpp>
#include <isi/util/file.hpp>
#include <isi/util/query.hpp>
#include <isi/util/aio.hpp>
#include <isi/util/error_handling.hpp>

#include <cstring>
#include <fcntl.h>

namespace isi::query::compact_index {
    aio::aio(const std::experimental::filesystem::path& path) : compact_index::base(path), m_ctx(0) {
        assert_exit(m_header.page_size() % isi::get_page_size() == 0, "page size needs to be divisible by 4096 so the index can be opened with O_DIRECT");

        m_offsets.resize(m_header.parameters().size());
        m_offsets[0] = m_smd.curr_pos;
        for (size_t i = 1; i < m_header.parameters().size(); i++) {
            m_offsets[i] = m_offsets[i - 1] + m_header.page_size() * m_header.parameters()[i - 1].signature_size;
        }

        m_fd = open_file(path, O_RDONLY | O_DIRECT);
        if (io_setup(65536, &m_ctx) < 0) {
            exit_error_errno("io_setup error");
        }

        for (size_t i = 0; i < m_iocbs.size(); i++) {
            m_iocbs[i].aio_fildes = m_fd;
            m_iocbs[i].aio_lio_opcode = IOCB_CMD_PREAD;
            m_iocbs[i].aio_nbytes = m_header.page_size();
            m_iocbpp[i] = m_iocbs.data() + i;
        }
    }

    aio::~aio() {
        close_file(m_fd);
        if (io_destroy(m_ctx) < 0) {
            exit_error_errno("io_destroy error");
        }
    }

    void aio::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        int64_t num_requests = m_header.parameters().size() * hashes.size();

        #pragma omp parallel for collapse(2)
        for (size_t i = 0; i < m_header.parameters().size(); i++) {
            for (size_t j = 0; j < hashes.size(); j++) {
                uint64_t index = i + j * m_header.parameters().size();
                uint64_t hash = hashes[j] % m_header.parameters()[i].signature_size;
                //todo rows does not need to be reallocated each time
                m_iocbs[index].aio_buf = (uint64_t) rows + index * m_header.page_size();
                m_iocbs[index].aio_offset = m_offsets[i] + hash * m_header.page_size();
            }
        }

        int ret = io_submit(m_ctx, num_requests, m_iocbpp.data());
        if (ret != num_requests) {
            if (ret < 0) {
                perror("io_submit error");
            } else {
                fprintf(stderr, "could not sumbit IOs");
            }
            exit_error_errno("io_submit error");
        }

        io_event events[num_requests];
        if (io_getevents(m_ctx, num_requests, num_requests, events, nullptr) < num_requests) {
            exit_error_errno("io_getevents error");
        }

        for(size_t i = 0; i < num_requests; i++) {
            if(events[i].res != m_header.page_size()) {
                std::cout << i << " " << std::strerror(-events[i].res) << std::endl;
            }
        }
    }
}
