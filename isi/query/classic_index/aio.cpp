#include <isi/query/classic_index/aio.hpp>
#include <isi/util/file.hpp>
#include <isi/util/query.hpp>
#include <isi/util/aio.hpp>
#include <isi/util/error_handling.hpp>

#include <cstring>

namespace isi::query::classic_index {

    aio::aio(const std::experimental::filesystem::path& path) : classic_index::base(path), m_iocbs(65536, iocb()), m_iocbpp(65536) {
        m_fd = open_file(path);

        m_ctx = 0;
        int ret = io_setup(65536, &m_ctx);
        if (ret < 0) {
            exit_error_errno("io_setup error");
        }

        for (size_t i = 0; i < m_iocbs.size(); i++) {
            m_iocbs[i].aio_fildes = m_fd;
            m_iocbs[i].aio_lio_opcode = IOCB_CMD_PREAD;
            m_iocbs[i].aio_nbytes = m_header.block_size();
            m_iocbpp[i] = m_iocbs.data() + i;
        }
    }

    aio::~aio() {
        close_file(m_fd);
        int ret = io_destroy(m_ctx);
        if (ret < 0) {
            exit_error_errno("io_destroy error");
        }
    }

    void aio::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        timer t;
        io_event events[hashes.size()];
        int ret;

        t.active("setup");
        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            m_iocbs[i].aio_buf = (uint64_t) rows + i * m_header.block_size();
            m_iocbs[i].aio_offset = hashes[i] % m_header.signature_size() * m_header.block_size();
        }

        t.active("io_submit");
        ret = io_submit(m_ctx, hashes.size(), m_iocbpp.data());
        if (ret != static_cast<int64_t>(hashes.size())) {
            if (ret < 0)
                perror("io_submit error");
            else
                fprintf(stderr, "could not sumbit IOs");
            exit_error_errno("io_submit error");
        }

        t.active("io_getevents");
        ret = io_getevents(m_ctx, hashes.size(), hashes.size(), events, nullptr);
        if (ret < hashes.size()) {
            exit_error_errno("io_getevents error");
        }
        t.stop();
        std::cout << t << std::endl;
    }
}