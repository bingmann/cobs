#pragma once

#include <cobs/file/classic_index_header.hpp>
#include <cobs/util/timer.hpp>
#include <cobs/query/base.hpp>

namespace cobs::query::classic_index {
    class base : public query::base<file::classic_index_header> {
    protected:
        explicit base(const std::experimental::filesystem::path& path);
        uint64_t num_hashes() const override;
        uint64_t block_size() const override;
        uint64_t counts_size() const override;
    public:
        virtual ~base() = default;
    };
}

