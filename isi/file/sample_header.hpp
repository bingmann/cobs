#pragma once

#include <isi/file/header.hpp>

namespace isi::file {
    class sample_header : public header<sample_header> {
    private:
        uint32_t m_kmer_size;
    protected:
        void serialize(std::ofstream& ofs) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
        sample_header() = default;
        explicit sample_header(uint32_t kmer_size);
        uint32_t kmer_size() const;
    };
}



