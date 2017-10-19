#pragma once


#include <server/base.hpp>
#include <file/abss_header.hpp>

namespace genome::server::abss {
    class base : public server::base<file::abss_header> {
    protected:
        size_t m_num_hashes;
        explicit base(const boost::filesystem::path& path);
    public:
        void search(const std::string& query, uint32_t kmer_size,
                   std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) override;
        ~base() = default;
    };
}



