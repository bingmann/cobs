#pragma once

#include <ostream>
#include "kmer.hpp"

namespace isi {
    template<unsigned int N>
    class sample {
    private:
        std::vector<kmer<N>> m_data;
    public:
        void print(std::ostream& ostream) const {
            for (size_t i = 0; i < m_data.size(); i++) {
                ostream << m_data[i] << std::endl;
            }
        }

        std::vector<kmer<N>>& data() {
            return m_data;
        }

        const std::vector<kmer<N>>& data() const {
            return m_data;
        }
    };
}
