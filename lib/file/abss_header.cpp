#include "abss_header.hpp"

namespace genome::file {
    const std::string abss_header::magic_word = "ABSS";
    const std::string abss_header::file_extension = ".g_abss";


    size_t abss_header::padding_size(uint64_t curr_stream_pos) const {
        return (m_page_size - ((curr_stream_pos + abss_header::magic_word.size()) % m_page_size)) % m_page_size;
    }

    void abss_header::serialize(std::ofstream& ofs) const {
        genome::serialize(ofs, (uint32_t) m_parameters.size(), m_page_size);
        ofs.flush();
        for (const auto& parameter: m_parameters) {
            genome::serialize(ofs, std::get<0>(parameter), std::get<1>(parameter), std::get<2>(parameter));
        }
        for (const auto& file_name: m_file_names) {
            ofs << file_name << std::endl;
        }

        ofs.flush();
        std::vector<char> padding(padding_size(ofs.tellp()));
        ofs.write(padding.data(), padding.size());
        ofs.flush();
    }

    void abss_header::deserialize(std::ifstream& ifs) {
        uint32_t parameters_size;
        size_t num_file_names = 0;
        genome::deserialize(ifs, parameters_size, m_page_size);
        m_parameters.resize(parameters_size);
        for (auto& parameter: m_parameters) {
            genome::deserialize(ifs, std::get<0>(parameter), std::get<1>(parameter), std::get<2>(parameter));
            num_file_names += 8 * std::get<1>(parameter);
        }

        m_file_names.resize(num_file_names);
        for (auto& file_name: m_file_names) {
            std::getline(ifs, file_name);
        }

        stream_metadata smd = get_stream_metadata(ifs);
        ifs.seekg(smd.curr_pos + padding_size(smd.curr_pos), std::ios::beg);
    }

    abss_header::abss_header(uint32_t page_size) : m_page_size(page_size) {}

    abss_header::abss_header(const std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>& parameters, const std::vector<std::string>& file_names, uint32_t page_size)
            : m_parameters(parameters), m_file_names(file_names), m_page_size(page_size) { }

    const std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>& abss_header::parameters() const {
        return m_parameters;
    }

    const std::vector<std::string>& abss_header::file_names() const {
        return m_file_names;
    }
}
