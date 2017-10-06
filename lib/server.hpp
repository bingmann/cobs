#pragma once

#include <file/bloom_filter_header.hpp>
#include <boost/filesystem/path.hpp>

namespace genome {
    class server {
    private:
        file::bloom_filter_header m_bfh;
        byte* m_data;
        std::ifstream m_ifs;
        long m_pos_data_beg;
        template<unsigned int N>
        void create_hashes(std::vector<size_t>& hashes, const std::string& query);
    public:
        explicit server(const boost::filesystem::path& path);
        server(const boost::filesystem::path& path, int a);
        template<unsigned int N>
        void search_bloom_filter(const std::string& query, std::vector<std::pair<std::string, double>>& result);
        template<unsigned int N>
        void search_bloom_filter_2(const std::string& query, std::vector<std::pair<std::string, double>>& result);
    };
}

#include "server.tpp"
