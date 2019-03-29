/*******************************************************************************
 * cobs/kmer_buffer.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_KMER_BUFFER_HEADER
#define COBS_KMER_BUFFER_HEADER

#include <cobs/file/kmer_buffer_header.hpp>
#include <cobs/kmer.hpp>

#include <algorithm>
#include <ostream>
#include <vector>

#include <tlx/die.hpp>

namespace cobs {

template <unsigned int N>
class KMerBuffer
{
private:
    std::vector<KMer<N> > m_data;

public:
    void print(std::ostream& ostream) const;

    std::vector<KMer<N> >& data() {
        return m_data;
    }
    const std::vector<KMer<N> >& data() const {
        return m_data;
    }

    size_t num_kmers() const {
        return m_data.size();
    }

    void sort_kmers() {
        std::sort(m_data.begin(), m_data.end());
    }

    void serialize(std::ostream& os, const std::string& name) const {
        os.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        KMerBufferHeader sh(name, N);
        sh.serialize(os);
        os.write(reinterpret_cast<const char*>(m_data.data()),
                 KMer<N>::size* m_data.size());
    }

    void serialize(const fs::path& p, const std::string& name) const {
        fs::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, name);
    }

    void deserialize(std::istream& is, KMerBufferHeader& h) {
        is.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        h.deserialize(is);
        die_unless(N == h.kmer_size());

        size_t size = get_stream_size(is);
        m_data.resize(size / KMer<N>::size);
        is.read(reinterpret_cast<char*>(m_data.data()), size);
    }

    void deserialize(const fs::path& p, KMerBufferHeader& h) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, h);
    }
};

template <unsigned int N>
void KMerBuffer<N>::print(std::ostream& ostream) const {
    for (size_t i = 0; i < m_data.size(); i++) {
        ostream << m_data[i] << std::endl;
    }
}

} // namespace cobs

#endif // !COBS_KMER_BUFFER_HEADER

/******************************************************************************/
