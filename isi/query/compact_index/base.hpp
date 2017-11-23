#pragma once

#include <isi/query/base.hpp>
#include <isi/file/compact_index_header.hpp>

namespace isi::query::compact_index {
    class base : public query::base<file::compact_index_header> {
    protected:
        size_t m_num_hashes;
        size_t m_block_size;
        explicit base(const std::experimental::filesystem::path& path);
        uint64_t num_hashes() const override;
        uint64_t block_size() const override;
        uint64_t max_hash_value() const override;
        uint64_t counts_size() const override;
    public:
        virtual ~base() = default;
    };
}



