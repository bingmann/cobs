#include "server_mmap.hpp"
#include <fcntl.h>
#include <sys/mman.h>

namespace genome {

    server_mmap::server_mmap(const boost::filesystem::path& path) : server() {
        std::ifstream ifs;
        m_bfh = file::deserialize_header<file::bloom_filter_header>(ifs, path);

        long start_pos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        ifs.close();

        int fd = open(path.string().data(), O_RDONLY, 0);
        assert(fd != -1);

//  | MAP_POPULATE
        m_data = start_pos + reinterpret_cast<byte*>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0));
    }
/*
    void server_mmap::get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        std::vector<byte> count(m_bfh.block_size(), 0xFF);
        auto* count_64 = reinterpret_cast<uint64_t*>(count.data());
        auto* counts_64 = reinterpret_cast<uint64_t*>(counts.data());

        std::vector<char> count_1(m_bfh.block_size() * hashes.size(), 'a');
        int a = 0;
        for (size_t i = 0; i < hashes.size(); i++) {
            m_timer.active("mmap_access");
            auto data_64 = reinterpret_cast<uint64_t*>(m_data + hashes[i] * m_bfh.block_size());
            size_t j = 1;
            while (j * 8 <= m_bfh.block_size()) {
                count_64[j - 1] &= data_64[j - 1];
                j++;
            }
            j = (j - 1) * 8;
            while (j < m_bfh.block_size()) {
                count[j] &= m_data[hashes[i] * m_bfh.block_size() + j];
                j++;
            }
            if (i % m_bfh.num_hashes() == m_bfh.num_hashes() - 1) {
                m_timer.active("compute_counts");
                for (size_t k = 0; k < m_bfh.block_size(); k++) {
                    counts_64[2 * k] += m_count_expansions[count[k] & 0xF];
                    counts_64[2 * k + 1] += m_count_expansions[count[k] >> 4];
                }
                std::fill(count.begin(), count.end(), 0xFF);
            }
        }
    }
    */
    void server_mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        m_timer.active("mmap_access");
        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            auto data_64 = reinterpret_cast<uint64_t*>(m_data + hashes[i] * m_bfh.block_size());
            auto rows_64 = reinterpret_cast<uint64_t*>(rows);
            size_t j = 1;
            while (j * 8 <= m_bfh.block_size()) {
                rows_64[j - 1] = data_64[j - 1];
                j++;
            }
            j = (j - 1) * 8;
            while (j < m_bfh.block_size()) {
                rows[j] = m_data[j];
                j++;
            }
            rows += m_bfh.block_size();
        }
    }

    void server_mmap::get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        std::vector<byte> count(m_bfh.block_size(), 0xFF);
        auto* count_64 = reinterpret_cast<uint64_t*>(count.data());
        auto* counts_64 = reinterpret_cast<uint64_t*>(counts.data());

        std::vector<char> rows(m_bfh.block_size() * hashes.size(), 'a');
        read_from_disk(hashes, rows.data());

        m_timer.active("compute_counts");
        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            char* rows_8 = rows.data() + i * m_bfh.block_size();
            auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
            size_t j = 1;
            while (j * 8 <= m_bfh.block_size()) {
                count_64[j - 1] &= rows_64[j - 1];
                j++;
            }
            j = (j - 1) * 8;
            while (j < m_bfh.block_size()) {
                count[j] &= rows_8[j];
                j++;
            }
            if (i % m_bfh.num_hashes() == m_bfh.num_hashes() - 1) {
                for (size_t k = 0; k < m_bfh.block_size(); k++) {
                    counts_64[2 * k] += m_count_expansions[count[k] & 0xF];
                    counts_64[2 * k + 1] += m_count_expansions[count[k] >> 4];
//                    counts_64[2 * k] += m_count_expansions[count_1[i * hashes.size() + k] & 0xF];
//                    counts_64[2 * k + 1] += m_count_expansions[count_1[i * hashes.size() + k] >> 4];
                }
                std::fill(count.begin(), count.end(), 0xFF);
            }
        }
    }
}
