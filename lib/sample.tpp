#pragma once

#include "sample.hpp"
#include <algorithm>

template<unsigned int N>
sample<N>::sample(std::function<bool(char*, unsigned int)> get_line) {
    init(get_line);
}

template<unsigned int N>
void sample<N>::init(std::function<bool(char*, unsigned int)> get_line) {
    m_data.resize(0);
    char buffer[128];
    while(get_line(buffer, 128)) {
        m_data.push_back(kmer<N>(buffer));
//        std::cout << kmer<N>(m_data.back()) << std::endl;
    }
    std::sort(m_data.begin(), m_data.end(), [](const auto& kmer1, const auto& kmer2) {
        for (int i = 0; i < kmer<N>::size; i++) {
            if(kmer1.data()[i] != kmer2.data()[i]) {
                return kmer1.data()[i] < kmer2.data()[i];
            }
        }
        return false;
    });
}

template<unsigned int N>
void sample<N>::print(std::ostream &ostream) const {
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

