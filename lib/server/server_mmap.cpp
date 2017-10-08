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

    void server_mmap::get_counts(const std::vector<size_t>& hashes, std::vector<size_t>& counts) {
        std::vector<byte> count(m_bfh.block_size(), 0xFF);
        auto* count_64 = reinterpret_cast<uint64_t*>(count.data());
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
                for (size_t j = 0; j < m_bfh.block_size(); j++) {
                    for (size_t k = 0; k < 8; k++) {
                        counts[8 * j + k] += (count[j] >> k) & 1;
                    }
                }
                std::fill(count.begin(), count.end(), 0xFF);
            }
        }
    }
}
