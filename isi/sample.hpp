#pragma once

#include <ostream>
#include <vector>
#include <isi/kmer.hpp>

namespace isi {
    template<unsigned int N>
    class sample {
    private:
        std::vector<kmer<N>> m_data;
    public:
        void print(std::ostream& ostream) const;
        std::vector<kmer<N>>& data();
        const std::vector<kmer<N>>& data() const;
    };
}

namespace isi {
    template<unsigned int N>
    void sample<N>::print(std::ostream& ostream) const {
        for (size_t i = 0; i < m_data.size(); i++) {
            ostream << m_data[i] << std::endl;
        }
    }

    template<unsigned int N>
    std::vector<kmer<N>>& sample<N>::data() {
        return m_data;
    }

    template<unsigned int N>
    const std::vector<kmer<N>>& sample<N>::data() const {
        return m_data;
    }
}
