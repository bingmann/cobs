#include "bloom_filter_header.hpp"
#include <helpers.hpp>

namespace genome::file {
    const std::string bloom_filter_header::magic_word = "BLOOM";
    const std::string bloom_filter_header::file_extension = ".g_blo";

    void bloom_filter_header::serialize(std::ofstream& ofs) const {
        genome::serialize(ofs, m_bloom_filter_size, m_block_size, m_num_hashes);
    }

    void bloom_filter_header::deserialize(std::ifstream& ifs) {
        genome::deserialize(ifs, m_bloom_filter_size, m_block_size, m_num_hashes);
    }

    bloom_filter_header::bloom_filter_header(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes)
            : m_bloom_filter_size(bloom_filter_size), m_block_size(block_size), m_num_hashes(num_hashes) {}

    uint64_t bloom_filter_header::bloom_filter_size() const {
        return m_bloom_filter_size;
    }

    uint64_t bloom_filter_header::block_size() const {
        return m_block_size;
    }

    uint64_t bloom_filter_header::num_hashes() const {
        return m_num_hashes;
    }
}
