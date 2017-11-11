#pragma once

#include <file/classic_index_header.hpp>
#include <timer.hpp>
#include <server/base.hpp>

namespace isi::server::classic_index {
    class base : public server::base<file::classic_index_header> {
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

