#pragma once


#include "header.hpp"

namespace genome::file {
    class msbf_header : public header<msbf_header> {
    private:
        std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> m_parameters;
        std::vector<std::string> m_file_names;
        uint32_t m_page_size;
        size_t padding_size(uint64_t curr_stream_pos) const;
    protected:
        void serialize(std::ofstream& ofs) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
        explicit msbf_header(uint32_t page_size = 16384);
        msbf_header(const std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>& parameters, const std::vector<std::string>& file_names, uint32_t page_size = 16384);
        const std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>& parameters() const;
        const std::vector<std::string>& file_names() const;
    };
}
