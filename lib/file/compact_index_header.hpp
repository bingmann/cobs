#pragma once


#include "header.hpp"

namespace isi::file {
    class compact_index_header : public header<compact_index_header> {
    public:
        struct parameter {
            uint64_t signature_size;
            uint64_t num_hashes;
        };
    private:
        std::vector<parameter> m_parameters;
        std::vector<std::string> m_file_names;
        uint64_t m_page_size;
        size_t padding_size(uint64_t curr_stream_pos) const;
    protected:
        void serialize(std::ofstream& ofs) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
        explicit compact_index_header(uint64_t page_size = 4096);
        compact_index_header(const std::vector<parameter>& parameters, const std::vector<std::string>& file_names, uint64_t page_size = 4096);
        const std::vector<parameter>& parameters() const;
        const std::vector<std::string>& file_names() const;
        uint64_t page_size() const;
    };
}
