#include <file/util.hpp>
#include "ifs.hpp"

namespace genome::server::bss {
    ifs::ifs(const boost::filesystem::path& path) : base() {
        m_bssh = file::deserialize_header<file::bss_header>(m_ifs, path);
        m_pos_data_beg = m_ifs.tellg();
    }

    void ifs::get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        genome::bss bss(hashes.size(), m_bssh.block_size(), m_bssh.num_hashes());
        auto data_ptr = reinterpret_cast<char*>(bss.data().data());
        m_timer.active("ifs_access");
        for (size_t i = 0; i < hashes.size(); i++) {
            m_ifs.seekg(m_pos_data_beg + m_bssh.block_size() * hashes[i]);
            m_ifs.read(data_ptr + i * m_bssh.block_size(), m_bssh.block_size());
        }

        m_timer.active("compute_counts");
        std::vector<byte> count(bss.block_size(), 0xFF);
        for (size_t i = 0; i < hashes.size(); i++) {
            for (size_t j = 0; j < bss.block_size(); j++) {
                count[j] &= bss.data()[i * bss.block_size() + j];
            }
            if (i % bss.num_hashes() == bss.num_hashes() - 1) {
                for (size_t j = 0; j < bss.block_size(); j++) {
                    for (size_t k = 0; k < 8; k++) {
                        counts[8 * j + k] += (count[j] >> k) & 1;
                    }
                }
                std::fill(count.begin(), count.end(), 0xFF);
            }
        }
    }
}
