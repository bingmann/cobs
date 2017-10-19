#pragma once

#include <file/bss_header.hpp>
#include <timer.hpp>

namespace genome::server::bss {
    class base {
    private:
        void create_hashes(std::vector<size_t>& hashes, const std::string& query, uint32_t kmer_size) const;
        void counts_to_result(const std::vector<uint16_t>& counts, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) const;
    protected:
        timer m_timer;
        file::bss_header m_bssh;
        virtual void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) = 0;
    public:
        void search_bss(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0);
        const timer& get_timer() const;
    };
}

