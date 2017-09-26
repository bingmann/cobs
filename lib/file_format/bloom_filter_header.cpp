#include "bloom_filter_header.hpp"
#include <helpers.hpp>

namespace genome::file_format {
    uint32_t bloom_filter_header::size() const {
        return m_size;
    }

    uint32_t bloom_filter_header::type() const {
        return m_type;
    }

    void bloom_filter_header::serialize_content(std::ostream& ost) const {
        genome::serialize(ost, bloom_filter_size, block_size, num_hashes);
    }

    bloom_filter_header::bloom_filter_header(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes)
            : bloom_filter_size(bloom_filter_size), block_size(block_size), num_hashes(num_hashes) {
//        static_assert(m_size == sizeof(bloom_filter_header));
    }
}
