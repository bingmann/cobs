#include "server_mmap.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <file/util.hpp>

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
        m_data = reinterpret_cast<byte*>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0));
//        assert(madvise(m_data, size, MADV_RANDOM) == 0);
        m_data += start_pos;
    }

    void server_mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            auto data_8 = m_data + hashes[i] * m_bfh.block_size();
            auto rows_8 = rows + i * m_bfh.block_size();
            auto data_64 = reinterpret_cast<uint64_t*>(data_8);
            auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
            size_t j = 0;
            while ((j + 1) * 8 <= m_bfh.block_size()) {
                rows_64[j] = data_64[j];
                j++;
            }
            j *= 8;
            while (j < m_bfh.block_size()) {
                rows_8[j] = data_8[j];
                j++;
            }
        }
    }

    void server_mmap::aggregate_rows(const std::vector<size_t>& hashes, std::vector<char>& rows) {
        #pragma omp parallel for
        for (uint64_t i = 0; i < hashes.size(); i += m_bfh.num_hashes()) {
            auto rows_8 = rows.data() + i * m_bfh.block_size();
            auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
            for (size_t j = 1; j < m_bfh.num_hashes(); j++) {
                auto rows_8_j = rows_8 + j * m_bfh.block_size();
                auto rows_64_j = reinterpret_cast<uint64_t*>(rows_8_j);
                size_t k = 0;
                while ((k + 1) * 8 <= m_bfh.block_size()) {
                    rows_64[k] &= rows_64_j[k];
                    k++;
                }
                k = k * 8;
                while (k < m_bfh.block_size()) {
                    rows_8[k] &= rows_8_j[k];
                    k++;
                }
            }
        }
    }

    void server_mmap::compute_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts, const std::vector<char>& rows) {
        auto counts_64 = reinterpret_cast<uint64_t*>(counts.data());
        #pragma omp parallel for
        for (uint64_t i = 0; i < hashes.size(); i += m_bfh.num_hashes()) {
            auto rows_8 = rows.data() + i * m_bfh.block_size();
            for (size_t k = 0; k < m_bfh.block_size(); k++) {
                counts_64[2 * k] += m_count_expansions[rows_8[k] & 0xF];
                counts_64[2 * k + 1] += m_count_expansions[rows_8[k] >> 4];
            }
        }
    }

    void server_mmap::get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        std::vector<char> rows(m_bfh.block_size() * hashes.size());
        m_timer.active("mmap_access");
        read_from_disk(hashes, rows.data());
        m_timer.active("aggregate_rows");
        aggregate_rows(hashes, rows);
        m_timer.active("compute_counts");
        compute_counts(hashes, counts, rows);
        //todo test if it is faster to combine these functions for better cache locality
    }
}
