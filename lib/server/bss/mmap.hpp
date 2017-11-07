#pragma once

#include "server/bss/base.hpp"


namespace genome::server::bss {
    class mmap : public base {
    private:
        byte* m_data;
        virtual void read_from_disk(const std::vector<size_t>& hashes, char* rows);
    protected:
        void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit mmap(const boost::filesystem::path& path);
    };
}
