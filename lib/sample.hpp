//
// Created by Florian Gauger on 09.09.2017.
//

#pragma once

#include <boost/serialization/access.hpp>
#include <ostream>
#include <istream>
#include "kmer.hpp"

template<unsigned int N>
class sample {
private:
    friend class boost::serialization::access;
    std::vector<byte> m_data;
    template<class Archive>
    void serialize(Archive & ar, unsigned int version);
public:
    sample() = default;
    sample(std::istream& is, std::function<bool(std::string&)> get_line);
    void init(std::istream& is, std::function<bool(std::string&)> get_line);
    size_t size() const;
    kmer<N> get(size_t i) const;
    void print(std::ostream& ostream) const;
    const std::vector<byte>& data() const;
};

#include "sample.tpp"