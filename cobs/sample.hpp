#pragma once

#include <ostream>
#include <vector>
#include <cobs/kmer.hpp>

namespace cobs {
    template<unsigned int N>
    class sample {
    private:
        std::vector<kmer<N>> m_data;
    public:
        void print(std::ostream& ostream) const;
        std::vector<kmer<N>>& data();
        const std::vector<kmer<N>>& data() const;

        void sort_samples() {
            std::sort(m_data.begin(), m_data.end());
        }
    };
}

namespace cobs {
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
