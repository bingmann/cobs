#pragma once

#include "base.hpp"


namespace isi::server::bss {
    class ifs : public base {
    private:
        std::ifstream m_ifs;
        long m_pos_data_beg;
    protected:
        void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit ifs(const std::experimental::filesystem::path& path);
    };
}

