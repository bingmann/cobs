#pragma once

#include "server.hpp"


namespace genome {
    class server_mmap : public server {
    private:
        byte* m_data;
    protected:
        void get_counts(const std::vector<size_t>& hashes, std::vector<size_t>& counts) override;
    public:
        explicit server_mmap(const boost::filesystem::path& path);
    };
}
