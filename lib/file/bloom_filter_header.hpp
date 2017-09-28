#pragma once

#include <fstream>
#include "header.hpp"

namespace genome::file {
    class bloom_filter_header : public header<bloom_filter_header> {
    private:
        uint64_t m_bloom_filter_size;
        uint64_t m_block_size;
        uint64_t m_num_hashes;
    protected:
        void serialize(std::ofstream& ofs) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
        bloom_filter_header() = default;
        bloom_filter_header(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes);
        uint64_t bloom_filter_size() const;
        uint64_t block_size() const;
        uint64_t num_hashes() const;
    };
}



