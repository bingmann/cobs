#pragma once

#include <file/bss_header.hpp>
#include <timer.hpp>
#include <server/base.hpp>

namespace isi::server::bss {
    class base : public server::base<file::bss_header> {
    protected:
        explicit base(const std::experimental::filesystem::path& path);
        uint64_t num_hashes() const override;
        uint64_t block_size() const override;
        uint64_t max_hash_value() const override;
        uint64_t counts_size() const override;
    public:
        virtual ~base() = default;
    };
}

