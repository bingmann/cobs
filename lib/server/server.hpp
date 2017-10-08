#pragma once

#include <file/bloom_filter_header.hpp>
#include <boost/filesystem/path.hpp>
#include <timer.hpp>

namespace genome {
    class server {
    private:
        std::ifstream m_ifs;
        long m_pos_data_beg;
        template<unsigned int N>
        void create_hashes(std::vector<size_t>& hashes, const std::string& query) const;
        void counts_to_result(const std::vector<size_t>& counts, size_t max_count, std::vector<std::pair<double, std::string>>& result) const;
    protected:
        timer m_timer;
        file::bloom_filter_header m_bfh;
        virtual void get_counts(const std::vector<size_t>& hashes, std::vector<size_t>& counts) = 0;
    public:
        template<unsigned int N>
        void search_bloom_filter(const std::string& query, std::vector<std::pair<double, std::string>>& result);
        const timer& timer() const;
    };
}

#include "server.ipp"
