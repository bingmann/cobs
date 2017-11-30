#include <isi/file/classic_index_header.hpp>
#include <isi/util/file.hpp>

namespace isi::file {
    const std::string classic_index_header::magic_word = "CLASSIC_INDEX";
    const std::string classic_index_header::file_extension = ".cla_idx.isi";

    void classic_index_header::serialize(std::ofstream& ofs) const {
        isi::serialize(ofs, (uint32_t) m_file_names.size(), m_signature_size, m_num_hashes);
        for (const auto& file_name: m_file_names) {
            ofs << file_name << std::endl;
        }
    }

    void classic_index_header::deserialize(std::ifstream& ifs) {
        uint32_t file_names_size;
        isi::deserialize(ifs, file_names_size, m_signature_size, m_num_hashes);
        m_file_names.resize(file_names_size);
        for (auto& file_name: m_file_names) {
            std::getline(ifs, file_name);
        }
    }

    classic_index_header::classic_index_header(uint64_t signature_size, uint64_t num_hashes, const std::vector<std::string>& file_names)
            : m_signature_size(signature_size), m_num_hashes(num_hashes), m_file_names(file_names) {}

    uint64_t classic_index_header::signature_size() const {
        return m_signature_size;
    }

    uint64_t classic_index_header::block_size() const {
        return (m_file_names.size() + 7) / 8;
    }

    uint64_t classic_index_header::num_hashes() const {
        return m_num_hashes;
    }

    const std::vector<std::string>& classic_index_header::file_names() const {
        return m_file_names;
    }

    std::vector<std::string>& classic_index_header::file_names() {
        return m_file_names;
    }
}
