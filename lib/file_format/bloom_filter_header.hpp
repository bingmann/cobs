#pragma once

#include "header.hpp"

namespace genome::file_format {
    class bloom_filter_header : public header {
    private:
        static const uint32_t m_type = 3;
        static const uint32_t m_size = 24;
        uint64_t bloom_filter_size;
        uint64_t block_size;
        uint64_t num_hashes;
    protected:
        uint32_t type() const override;
        uint32_t size() const override;
        void serialize_content(std::ostream& stream) const override;
    public:
        bloom_filter_header(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes);
    };
}



