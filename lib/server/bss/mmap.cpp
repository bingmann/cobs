#include "mmap.hpp"
#include <file/util.hpp>
#include <server/util.hpp>
#include <cstring>

namespace genome::server::bss {

    mmap::mmap(const std::experimental::filesystem::path& path) : bss::base(path) {
        m_data = initialize_mmap(path, m_smd);
//        assert(madvise(m_data, size, MADV_RANDOM) == 0);
    }

    void mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            auto data_8 = m_data + hashes[i] * m_header.block_size();
            auto rows_8 = rows + i * m_header.block_size();
            std::memcpy(rows_8, data_8, m_header.block_size());
        }
    }

    void mmap::calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        std::vector<char> rows(m_header.block_size() * hashes.size());
        m_timer.active("mmap_access");
        read_from_disk(hashes, rows.data());
        m_timer.active("aggregate_rows");
        aggregate_rows(hashes.size(), rows.data());
        m_timer.active("compute_counts");
        compute_counts(hashes.size(), counts, rows.data());
        //todo test if it is faster to combine these functions for better cache locality
    }
}
