#pragma once

#include <fstream>
#include "header.hpp"
#include <string>
#include <vector>

namespace genome::file {
    class bloom_filter_header : public header<bloom_filter_header> {
    private:
        uint64_t m_bloom_filter_size;
        uint64_t m_block_size;
        uint64_t m_num_hashes;
        std::vector<std::string> m_file_names;
    protected:
        void serialize(std::ofstream& ofs) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
        bloom_filter_header();
        bloom_filter_header(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes, const std::vector<std::string>& file_names);
        uint64_t bloom_filter_size() const;
        uint64_t block_size() const;
        uint64_t num_hashes() const;
        const std::vector<std::string>& file_names() const;
    };
}



