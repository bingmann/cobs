#include <server/util.hpp>
#include "mmap.hpp"

namespace genome::server::abss {
    mmap::mmap(const boost::filesystem::path& path) : abss::base(path) {
        m_data = initialize_mmap(path, m_smd);
//        assert(madvise(m_data, size, MADV_RANDOM) == 0);
    }

    void mmap::calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {

    }
}
