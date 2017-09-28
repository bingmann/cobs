#include "sample.hpp"
#include "util.hpp"
#include "frequency_header.hpp"
#include "bloom_filter_header.hpp"
#include "sample_header.hpp"

namespace genome::file {
    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s) {
        sample_header sh(31);
        header<sample_header>::serialize(ofs, sh);
        ofs.write(reinterpret_cast<const char*>(s.data().data()), kmer<N>::size * s.data().size());
    }

    template<uint32_t N>
    void serialize(const boost::filesystem::path& p, const sample<N>& s) {
        boost::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, s);
    }

    inline void serialize(std::ofstream& ofs, const bloom_filter& bf) {
        bloom_filter_header bfh(bf.bloom_filter_size(), bf.block_size(), bf.num_hashes());
        header<bloom_filter_header>::serialize(ofs, bfh);
        ofs.write(reinterpret_cast<const char*>(bf.data().data()), bf.data().size());
    }

    inline void serialize(const boost::filesystem::path& p, const bloom_filter& bf) {
        boost::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, bf);
    }

    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s) {
        sample_header h;
        header<sample_header>::deserialize(ifs, h);
        assert(N == h.kmer_size());

        std::streamoff pos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg() - pos;
        ifs.seekg(pos, std::ios::beg);
        s.data().resize(size / kmer<N>::size);
        ifs.read(reinterpret_cast<char*>(s.data().data()), size);
    }

    template<uint32_t N>
    void deserialize(const boost::filesystem::path& p, sample<N>& s) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, s);
    }

    inline void deserialize(std::ifstream& ifs, bloom_filter& bf) {
        bloom_filter_header h;
        header<bloom_filter_header>::deserialize(ifs, h);
        bf.bloom_filter_size(h.bloom_filter_size());
        bf.block_size(h.block_size());
        bf.num_hashes(h.num_hashes());

        std::streamoff pos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg() - pos;
        ifs.seekg(pos, std::ios::beg);
        bf.data().resize(size);
        ifs.read(reinterpret_cast<char*>(bf.data().data()), size);
    }

    inline void deserialize(const boost::filesystem::path& p, bloom_filter& bf) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, bf);
    }

    template<class T>
    void serialize_header(std::ofstream& ofs, const boost::filesystem::path& p) {
        ofs.open(p.string(), std::ios::out | std::ios::binary);
        T h;
        header<T>::serialize(ofs, h);
    }

    template<class T>
    void deserialize_header(std::ifstream& ifs, const boost::filesystem::path& p) {
        ifs.open(p.string(), std::ios::in | std::ios::binary);
        T h;
        header<T>::deserialize(ifs, h);
    }
}
