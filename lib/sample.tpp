#pragma once

#include "sample.hpp"

template<unsigned int N>
sample<N>::sample(std::istream &is, std::function<bool(std::string &)> get_line) {
    init(is, get_line);
}

template<unsigned int N>
void sample<N>::init(std::istream &is, std::function<bool(std::string&)> get_line) {
    m_data.resize(0);
    std::string line;
    std::back_insert_iterator<std::vector<byte>> iter(m_data);
    size_t s = 0;
    while(get_line(line)) {
        kmer<N>::initialize_data(line.c_str(), iter);
//        std::cout << line << std::endl << get(s) << std::endl << std::flush;
        s++;
    }
}

template<unsigned int N>
template<class Archive>
void sample<N>::serialize(Archive &ar, unsigned int version) {
    ar & m_data;
}

template<unsigned int N>
size_t sample<N>::size() const {
    return m_data.size() / kmer<N>::size();
}

template<unsigned int N>
kmer<N> sample<N>::get(size_t i) const {
    return kmer<N>(m_data.data() + i * kmer<N>::size(), m_data.data() + (i + 1) * kmer<N>::size());
}

template<unsigned int N>
void sample<N>::print(std::ostream &ostream) const {
    for (size_t i = 0; i < size(); i++) {
        ostream << get(i) << std::endl;
    }
}

template<unsigned int N>
const std::vector<byte> &sample<N>::data() const {
    return m_data;
}
