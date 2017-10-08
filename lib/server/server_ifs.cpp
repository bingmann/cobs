#include "server_ifs.hpp"

namespace genome {
    server_ifs::server_ifs(const boost::filesystem::path& path) : server() {
        m_bfh = file::deserialize_header<file::bloom_filter_header>(m_ifs, path);
        m_pos_data_beg = m_ifs.tellg();
    }

    void server_ifs::get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        bloom_filter bf(hashes.size(), m_bfh.block_size(), m_bfh.num_hashes());
        auto data_ptr = reinterpret_cast<char*>(bf.data().data());
        m_timer.active("ifs_access");
        for (size_t i = 0; i < hashes.size(); i++) {
            m_ifs.seekg(m_pos_data_beg + m_bfh.block_size() * hashes[i]);
            m_ifs.read(data_ptr + i * m_bfh.block_size(), m_bfh.block_size());
        }

        m_timer.active("compute_counts");
        std::vector<byte> count(bf.block_size(), 0xFF);
        for (size_t i = 0; i < hashes.size(); i++) {
            for (size_t j = 0; j < bf.block_size(); j++) {
                count[j] &= bf.data()[i * bf.block_size() + j];
            }
            if (i % bf.num_hashes() == bf.num_hashes() - 1) {
                for (size_t j = 0; j < bf.block_size(); j++) {
                    for (size_t k = 0; k < 8; k++) {
                        counts[8 * j + k] += (count[j] >> k) & 1;
                    }
                }
                std::fill(count.begin(), count.end(), 0xFF);
            }
        }
    }
}
