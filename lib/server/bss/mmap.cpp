#include "mmap.hpp"
#include <file/util.hpp>
#include <server/util.hpp>

namespace genome::server::bss {

    mmap::mmap(const boost::filesystem::path& path) : bss::base(path) {
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

    void mmap::aggregate_rows(const std::vector<size_t>& hashes, std::vector<char>& rows) {
        #pragma omp parallel for
        for (uint64_t i = 0; i < hashes.size(); i += m_header.num_hashes()) {
            auto rows_8 = rows.data() + i * m_header.block_size();
            auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
            for (size_t j = 1; j < m_header.num_hashes(); j++) {
                auto rows_8_j = rows_8 + j * m_header.block_size();
                auto rows_64_j = reinterpret_cast<uint64_t*>(rows_8_j);
                size_t k = 0;
                while ((k + 1) * 8 <= m_header.block_size()) {
                    rows_64[k] &= rows_64_j[k];
                    k++;
                }
                k = k * 8;
                while (k < m_header.block_size()) {
                    rows_8[k] &= rows_8_j[k];
                    k++;
                }
            }
        }
    }

    void mmap::compute_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts, const std::vector<char>& rows) {
        auto counts_64 = reinterpret_cast<uint64_t*>(counts.data());
        #pragma omp parallel for
        for (size_t k = 0; k < m_header.block_size(); k++) {
            for (uint64_t i = 0; i < hashes.size(); i += m_header.num_hashes()) {
            auto rows_8 = rows.data() + i * m_header.block_size();
                counts_64[2 * k] += m_count_expansions[rows_8[k] & 0xF];
                counts_64[2 * k + 1] += m_count_expansions[rows_8[k] >> 4];
            }
        }
    }

    void mmap::calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        std::vector<char> rows(m_header.block_size() * hashes.size());
        m_timer.active("mmap_access");
        read_from_disk(hashes, rows.data());
        m_timer.active("aggregate_rows");
        aggregate_rows(hashes, rows);
        m_timer.active("compute_counts");
        compute_counts(hashes, counts, rows);
        //todo test if it is faster to combine these functions for better cache locality
    }
}
