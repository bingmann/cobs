#pragma once


#include <server/compact_index/base.hpp>

namespace isi::server::compact_index {
    class mmap : public base {
    private:
        int m_fd;
        std::vector<uint8_t*> m_data;
        virtual void read_from_disk(const std::vector<size_t>& hashes, char* rows);
    protected:
        void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit mmap(const std::experimental::filesystem::path& path);
        ~mmap();
    };
}
