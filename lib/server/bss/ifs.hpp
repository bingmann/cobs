#pragma once

#include "base.hpp"


namespace genome::server::bss {
    class ifs : public base {
    private:
        std::ifstream m_ifs;
        long m_pos_data_beg;
    protected:
        void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit ifs(const boost::filesystem::path& path);
    };
}

