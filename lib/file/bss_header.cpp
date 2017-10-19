#include "bss_header.hpp"
#include <util.hpp>

namespace genome::file {
    const std::string bss_header::magic_word = "BSS";
    const std::string bss_header::file_extension = ".g_bss";

    void bss_header::serialize(std::ofstream& ofs) const {
        genome::serialize(ofs, m_signature_size, m_block_size, m_num_hashes);
        for(const auto& file_name: m_file_names) {
            ofs << file_name << std::endl;
        }
    }

    void bss_header::deserialize(std::ifstream& ifs) {
        genome::deserialize(ifs, m_signature_size, m_block_size, m_num_hashes);
        m_file_names.resize(8 * m_block_size);
        for (auto& file_name: m_file_names) {
            std::getline(ifs, file_name);
        }
    }

    bss_header::bss_header() {}

    bss_header::bss_header(uint64_t signature_size, uint64_t block_size, uint64_t num_hashes, const std::vector<std::string>& file_names)
            : m_signature_size(signature_size), m_block_size(block_size), m_num_hashes(num_hashes), m_file_names(file_names) {}

    uint64_t bss_header::signature_size() const {
        return m_signature_size;
    }

    uint64_t bss_header::block_size() const {
        return m_block_size;
    }

    uint64_t bss_header::num_hashes() const {
        return m_num_hashes;
    }

    const std::vector<std::string>& bss_header::file_names() const {
        return m_file_names;
    }
}
