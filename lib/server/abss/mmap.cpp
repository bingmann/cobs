#include <server/util.hpp>
#include "mmap.hpp"

namespace genome::server::abss {
    mmap::mmap(const boost::filesystem::path& path) : abss::base(path) {
        m_data.reserve(m_header.parameters().size());
        m_data[0] = initialize_mmap(path, m_smd);
        for (size_t i = 1; i < m_header.parameters().size(); i++) {
            m_data[i] = m_data[i - 1] + m_header.page_size() * m_header.parameters()[i - 1].signature_size;
        }
//        assert(madvise(m_data, size, MADV_RANDOM) == 0);
    }

    void mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for collapse(2)
        for (size_t i = 0; i < m_header.parameters().size(); i++) {
            for (size_t j = 0; j < hashes.size(); j++) {
                uint64_t hash = hashes[j] % m_header.parameters()[i].signature_size;
                auto data_8 = m_data[i] + hash * m_header.page_size();
                auto rows_8 = rows + (i + j * m_header.parameters().size()) * m_header.page_size();
                std::memcpy(rows_8, data_8, m_header.page_size());
            }
        }
    }

    void mmap::aggregate_rows(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for
        for (uint64_t i = 0; i < hashes.size(); i += num_hashes()) {
            auto rows_8 = rows + i * m_block_size;
            auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
            for (size_t j = 1; j < num_hashes(); j++) {
                auto rows_8_j = rows_8 + j * m_block_size;
                auto rows_64_j = reinterpret_cast<uint64_t*>(rows_8_j);
                size_t k = 0;
                while ((k + 1) * 8 <= m_block_size) {
                    rows_64[k] &= rows_64_j[k];
                    k++;
                }
                k = k * 8;
                while (k < m_block_size) {
                    rows_8[k] &= rows_8_j[k];
                    k++;
                }
            }
        }
    }

    void mmap::compute_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts, const byte* rows) {
        auto counts_64 = reinterpret_cast<uint64_t*>(counts.data());
        #pragma omp parallel for
        for (size_t k = 0; k < m_block_size; k++) {
            for (uint64_t i = 0; i < hashes.size(); i += num_hashes()) {
                auto rows_8 = rows + i * m_block_size;
                counts_64[2 * k] += m_count_expansions[rows_8[k] & 0xF];
                counts_64[2 * k + 1] += m_count_expansions[rows_8[k] >> 4];
            }
        }
    }

    void mmap::calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        std::vector<char> rows(hashes.size() * m_block_size);
        m_timer.active("mmap_access");
        read_from_disk(hashes, rows.data());
        m_timer.active("aggregate_rows");
        aggregate_rows(hashes, rows.data());
        m_timer.active("compute_counts");
        compute_counts(hashes, counts, reinterpret_cast<byte*>(rows.data()));
        //todo test if it is faster to combine these functions for better cache locality
    }
}
