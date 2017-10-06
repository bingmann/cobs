//
// Created by Florian Gauger on 09.09.2017.
//

#pragma once

#include <ostream>
#include "kmer.hpp"

namespace genome {
    template<unsigned int N>
    class sample {
    private:
        std::vector<kmer<N>> m_data;
    public:
        sample() = default;
        void print(std::ostream& ostream) const;
        std::vector<kmer<N>>& data();
        const std::vector<kmer<N>>& data() const;
    };
}

#include "sample.tpp"