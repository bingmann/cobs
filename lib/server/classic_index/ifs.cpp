#include <file/util.hpp>
#include "ifs.hpp"

namespace isi::server::classic_index {
    ifs::ifs(const std::experimental::filesystem::path& path) : classic_index::base(path) {}

    void ifs::calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) {
        isi::classic_index classic_index(hashes.size(), m_header.block_size(), m_header.num_hashes());
        auto data_ptr = reinterpret_cast<char*>(classic_index.data().data());
        m_timer.active("ifs_access");
        for (size_t i = 0; i < hashes.size(); i++) {
            m_ifs.seekg(m_pos_data_beg + m_header.block_size() * hashes[i]);
            m_ifs.read(data_ptr + i * m_header.block_size(), m_header.block_size());
        }

        m_timer.active("compute_counts");
        std::vector<byte> count(classic_index.block_size(), 0xFF);
        for (size_t i = 0; i < hashes.size(); i++) {
            for (size_t j = 0; j < classic_index.block_size(); j++) {
                count[j] &= classic_index.data()[i * classic_index.block_size() + j];
            }
            if (i % classic_index.num_hashes() == classic_index.num_hashes() - 1) {
                for (size_t j = 0; j < classic_index.block_size(); j++) {
                    for (size_t k = 0; k < 8; k++) {
                        counts[8 * j + k] += (count[j] >> k) & 1;
                    }
                }
                std::fill(count.begin(), count.end(), 0xFF);
            }
        }
    }
}
