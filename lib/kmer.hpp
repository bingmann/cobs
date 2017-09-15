//
// Created by Florian Gauger on 08.09.2017.
//

#pragma once

#include <array>
#include <string>
#include <map>
#include <cstddef>
#include <boost/array.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <iostream>
#include "helpers.hpp"


template<unsigned int N>
class kmer {
public:
    static_assert( N < 33U, "Maximum k-mer size is 32");
    using data_type = typename selectTypeByDim<N, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>::type;
private:
    friend class boost::serialization::access;
    static const std::map<unsigned int, byte> m_bps_to_byte;
    static const std::map<byte, std::string> m_byte_to_bps;
    data_type m_data;

    void initialize_map();
    static unsigned int chars_to_int(char c1, char c2, char c3, char c4);
    template<class Archive>
    void serialize(Archive & ar, unsigned int version);
public:
    kmer();
    explicit kmer(data_type data);
    template<typename Iterator>
    kmer(Iterator begin, Iterator end);
    explicit kmer(const char* kmer_string);
    static typename kmer<N>::data_type initialize_data(const char* kmer_string);
    byte* data();
    void print(std::ostream& ostream) const;
    explicit operator data_type();
};

#include "kmer.tpp"
