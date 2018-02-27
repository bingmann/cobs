#pragma once

#include <cobs/query/compact_index/base.hpp>

namespace cobs::query::compact_index {
    class mmap : public base {
    private:
        int m_fd;
        std::vector<uint8_t*> m_data;
    protected:
        void read_from_disk(const std::vector<size_t>& hashes, char* rows) override;
    public:
        explicit mmap(const std::experimental::filesystem::path& path);
        ~mmap();
    };
}
