//
// Created by Florian Gauger on 09.09.2017.
//

#pragma once

#include <boost/serialization/access.hpp>
#include <ostream>
#include <istream>
#include "kmer.hpp"
#include <kmer.hpp>

template<unsigned int N>
class sample {
private:
    friend class boost::serialization::access;
    std::vector<typename kmer<N>::data_type> m_data;
    template<class Archive>
    void serialize(Archive & ar, unsigned int version);
public:
    sample() = default;
    sample(std::function<bool(char*, unsigned int)> get_line);
    void init(std::function<bool(char*, unsigned int)> get_line);
    size_t size() const;
    kmer<N> get(size_t i) const;
    void print(std::ostream& ostream) const;
    const std::vector<typename kmer<N>::data_type>& data() const;
};

#include "sample.tpp"