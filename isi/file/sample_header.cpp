#include <isi/file/util.hpp>
#include <isi/file/sample_header.hpp>


namespace isi::file {
    const std::string sample_header::magic_word = "SAMPLE";
    const std::string sample_header::file_extension = ".g_sam";

    void sample_header::serialize(std::ofstream& ofs) const {
        isi::serialize(ofs, m_kmer_size);
    }

    void sample_header::deserialize(std::ifstream& ifs) {
        isi::deserialize(ifs, m_kmer_size);
    }

    sample_header::sample_header(uint32_t kmer_size) : m_kmer_size(kmer_size) {}

    uint32_t sample_header::kmer_size() const {
        return m_kmer_size;
    }
}
