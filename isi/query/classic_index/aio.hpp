#pragma once

#include <isi/query/classic_index/base.hpp>
#include <linux/aio_abi.h>

namespace isi::query::classic_index {
    class aio : public base {
    private:
        int m_fd;
        aio_context_t m_ctx;
        std::vector<iocb> m_iocbs;
        std::vector<iocb*> m_iocbpp;
    protected:
        virtual void read_from_disk(const std::vector<size_t>& hashes, char* rows);
    public:
        explicit aio(const std::experimental::filesystem::path& path);
        ~aio();
    };
}
