#pragma once

#include "server.hpp"


namespace genome {
    class server_ifs : public server {
    private:
        std::ifstream m_ifs;
        long m_pos_data_beg;
    protected:
        void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit server_ifs(const boost::filesystem::path& path);
    };
}

