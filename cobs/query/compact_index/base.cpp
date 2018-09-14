#include <cassert>
#include <cobs/query/compact_index/base.hpp>

namespace cobs::query::compact_index {
    base::base(const fs::path& path) : query::base<file::compact_index_header>(path) {
        //todo assertions that all the data in the header is correct
        m_block_size = m_header.page_size() * m_header.parameters().size();
        m_num_hashes = m_header.parameters()[0].num_hashes;
        for (const auto& p: m_header.parameters()) {
            assert(m_num_hashes == p.num_hashes);
        }
    }

    uint64_t base::num_hashes() const {
        return m_num_hashes;
    }

    uint64_t base::block_size() const {
        return m_block_size;
    }

    uint64_t base::counts_size() const {
        return 8 * m_header.parameters().size() * m_header.page_size();
    }
}
