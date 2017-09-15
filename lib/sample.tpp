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
        m_data.push_back(kmer<N>::initialize_data(buffer));
//        std::cout << kmer<N>(m_data.back()) << std::endl;
    }
    std::sort(m_data.begin(), m_data.end());
}

template<unsigned int N>
template<class Archive>
void sample<N>::serialize(Archive &ar, unsigned int version) {
    ar & m_data;
}

template<unsigned int N>
size_t sample<N>::size() const {
    return m_data.size();
}

template<unsigned int N>
kmer<N> sample<N>::get(size_t i) const {
    return kmer<N>(m_data[i]);
}

template<unsigned int N>
void sample<N>::print(std::ostream &ostream) const {
    for (size_t i = 0; i < size(); i++) {
        ostream << get(i) << std::endl;
    }
}

template<unsigned int N>
const std::vector<typename kmer<N>::data_type> &sample<N>::data() const {
    return m_data;
}
