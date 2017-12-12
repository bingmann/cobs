#include <isi/query/classic_index/aio.hpp>
#include <isi/util/file.hpp>
#include <isi/util/query.hpp>
#include <isi/util/aio.hpp>
#include <isi/util/error_handling.hpp>

#include <cstring>

namespace isi::query::classic_index {

    aio::aio(const std::experimental::filesystem::path& path) : classic_index::base(path) {
        m_fd = open_file(path);
    }

    aio::~aio() {
        close_file(m_fd);
    }

    void aio::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        aio_context_t ctx;
        io_event events[hashes.size()];
        int ret;

        ctx = 0;
        ret = io_setup(65536, &ctx);
        if (ret < 0) {
            exit_error_errno("io_setup error");
        }

        std::vector<iocb> iocbs(hashes.size(), iocb());
        std::vector<iocb*> iocbpp;

        for (size_t i = 0; i < hashes.size(); i++) {
            iocbs[i].aio_fildes = m_fd;
            iocbs[i].aio_lio_opcode = IOCB_CMD_PREAD;
            iocbs[i].aio_buf = (uint64_t) rows + i * m_header.block_size();
            iocbs[i].aio_offset = hashes[i] % m_header.signature_size() * m_header.block_size();
            iocbs[i].aio_nbytes = m_header.block_size();
            iocbpp.push_back(iocbs.data() + i);
        }

        std::cout << "aio_nbytes: " << iocbpp[0][0].aio_buf << std::endl;
        std::cout << "aio_nbytes: " << iocbpp[1][0].aio_buf << std::endl;

        ret = io_submit(ctx, iocbpp.size(), iocbpp.data());
        if (ret != static_cast<int64_t>(iocbpp.size())) {
            if (ret < 0)
                perror("io_submit error");
            else
                fprintf(stderr, "could not sumbit IOs");
            exit_error_errno("io_submit error");
        }
        std::cout << "after submit" << std::endl;

        /* get the reply */
        ret = io_getevents(ctx, iocbpp.size(), iocbpp.size(), events, nullptr);
        std::cout << "ret: " << ret << std::endl;

        ret = io_destroy(ctx);
        if (ret < 0) {
            exit_error_errno("io_destroy error");
        }
    }
}
