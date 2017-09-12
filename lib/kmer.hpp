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
private:
    friend class boost::serialization::access;
    static const std::map<unsigned int, byte> m_bps_to_byte;
    static const std::map<byte, std::string> m_byte_to_bps;
    static const size_t m_size = (N + 3) / 4;
    std::array<byte, m_size> m_data;

    void initialize_map();
    static unsigned int chars_to_int(char c1, char c2, char c3, char c4);
    template<class Archive>
    void serialize(Archive & ar, unsigned int version);
public:
    kmer();
    template<typename Iterator>
    kmer(Iterator begin, Iterator end);
    explicit kmer(const char* kmer_string);
    template <typename OutputIterator>
    static void initialize_data(const char* kmer_string, OutputIterator outputIterator);
    static size_t size();
    byte* data();
    void print(std::ostream& ostream) const;
};

#include "kmer.tpp"
