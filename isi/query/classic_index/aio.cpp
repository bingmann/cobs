#include <isi/query/classic_index/aio.hpp>
#include <isi/util/file.hpp>
#include <isi/util/query.hpp>
#include <isi/util/aio.hpp>
#include <isi/util/error_handling.hpp>

#include <cstring>

namespace isi::query::classic_index {

    aio::aio(const std::experimental::filesystem::path& path) : classic_index::base(path), m_iocbs(65536, iocb()), m_iocbpp(65536) {
        m_fd = open_file(path);
        
        for (size_t i = 0; i < hashes.size(); i++) {
            m_iocbs[i].aio_fildes = m_fd;
            m_iocbs[i].aio_lio_opcode = IOCB_CMD_PREAD;
            m_iocbs[i].aio_offset = hashes[i] % m_header.signature_size() * m_header.block_size();
            m_iocbs[i].aio_nbytes = m_header.block_size();
            m_iocbpp[i] = m_iocbs.data() + i;
        }
    }

    aio::~aio() {
        close_file(m_fd);
    }

    void aio::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        timer t;
        t.active("setup");
        aio_context_t ctx;
        io_event events[hashes.size()];
        int ret;

        ctx = 0;
        ret = io_setup(65536, &ctx);
        if (ret < 0) {
            exit_error_errno("io_setup error");
        }

        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            m_iocbs[i].aio_buf = (uint64_t) rows + i * m_header.block_size();
            m_iocbs[i].aio_offset = hashes[i] % m_header.signature_size() * m_header.block_size();
        }

        ret = io_submit(ctx, hashes.size(), m_iocbpp.data());
        t.active("io");
        if (ret != static_cast<int64_t>(hashes.size())) {
            if (ret < 0)
                perror("io_submit error");
            else
                fprintf(stderr, "could not sumbit IOs");
            exit_error_errno("io_submit error");
        }

        ret = io_getevents(ctx, hashes.size(), hashes.size(), events, nullptr);
        if (ret < hashes.size()) {
            exit_error_errno("io_getevents error");
        }

        ret = io_destroy(ctx);
        if (ret < 0) {
            exit_error_errno("io_destroy error");
        }
        t.stop();
        std::cout << t << std::endl;
    }
}