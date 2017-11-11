#pragma once

#include <fstream>
#include "header.hpp"
#include <string>
#include <vector>

namespace isi::file {
    class bss_header : public header<bss_header> {
    private:
        uint64_t m_signature_size;
        uint64_t m_block_size;
        uint64_t m_num_hashes;
        std::vector<std::string> m_file_names;
    protected:
        void serialize(std::ofstream& ofs) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
        bss_header();
        bss_header(uint64_t signature_size, uint64_t block_size, uint64_t num_hashes, const std::vector<std::string>& file_names);
        uint64_t signature_size() const;
        uint64_t block_size() const;
        uint64_t num_hashes() const;
        const std::vector<std::string>& file_names() const;
    };
}



