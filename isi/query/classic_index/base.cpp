#include <isi/query/classic_index/base.hpp>

namespace isi::query::classic_index {
    base::base(const std::experimental::filesystem::path& path) : query::base<file::classic_index_header>(path) { }

    uint64_t base::num_hashes() const {
            return m_header.num_hashes();
    }

    uint64_t base::block_size() const {
        return m_header.block_size();
    }

    uint64_t base::counts_size() const {
            return 8 * m_header.block_size();
    }
}
