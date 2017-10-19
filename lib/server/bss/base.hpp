#pragma once

#include <file/bss_header.hpp>
#include <timer.hpp>
#include <server/base.hpp>

namespace genome::server::bss {
    class base : public server::base<file::bss_header> {
    protected:
        virtual void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) = 0;
        explicit base(const boost::filesystem::path& path);
    public:
        void search(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0) override;
        virtual ~base() = default;
    };
}

