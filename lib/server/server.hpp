#pragma once

#include <file/bloom_filter_header.hpp>
#include <boost/filesystem/path.hpp>
#include <timer.hpp>
#include <iostream>

namespace genome {
    class server {
    private:
        std::ifstream m_ifs;
        long m_pos_data_beg;
        template<unsigned int N>
        void create_hashes(std::vector<size_t>& hashes, const std::string& query) const;
        void counts_to_result(const std::vector<uint16_t>& counts, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) const;
    protected:
        timer m_timer;
        static const uint64_t m_count_expansions[16];
        file::bloom_filter_header m_bfh;
        virtual void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) = 0;
    public:
        template<unsigned int N>
        void search_bloom_filter(const std::string& query, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0);
        const timer& get_timer() const;
    };
}

#include "server.ipp"
