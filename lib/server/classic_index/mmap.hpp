#pragma once

#include "server/classic_index/base.hpp"


namespace isi::server::classic_index {
    class mmap : public base {
    private:
        byte* m_data;
        virtual void read_from_disk(const std::vector<size_t>& hashes, char* rows);
    protected:
        void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit mmap(const std::experimental::filesystem::path& path);
    };
}
