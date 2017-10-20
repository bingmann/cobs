#pragma once


#include <server/base.hpp>
#include <file/abss_header.hpp>

namespace genome::server::abss {
    class base : public server::base<file::abss_header> {
    protected:
        size_t m_num_hashes;
        explicit base(const boost::filesystem::path& path);
        uint64_t num_hashes() const override;
        uint64_t max_hash_value() const override;
        uint64_t counts_size() const override;
    public:
        virtual ~base() = default;
    };
}



