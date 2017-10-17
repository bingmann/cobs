#pragma once

#include <file/bloom_filter_header.hpp>
#include <timer.hpp>

namespace genome {
    class server {
    private:
        void create_hashes(std::vector<size_t>& hashes, const std::string& query, uint32_t kmer_size) const;
        void counts_to_result(const std::vector<uint16_t>& counts, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) const;
    protected:
        timer m_timer;
        static const uint64_t m_count_expansions[16];
        file::bloom_filter_header m_bfh;
        virtual void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) = 0;
    public:
        void search_bloom_filter(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0);
        const timer& get_timer() const;
    };
}

