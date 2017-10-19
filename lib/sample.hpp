#pragma once

#include <ostream>
#include "kmer.hpp"

namespace genome {
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

#include "sample.tpp"