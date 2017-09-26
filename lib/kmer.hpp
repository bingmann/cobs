//
// Created by Florian Gauger on 08.09.2017.
//

#pragma once

#include <array>
#include <string>
#include <map>
#include <ostream>
#include "helpers.hpp"


namespace genome {
    template<unsigned int N>
    class kmer {
    public:
        static const size_t size = (N + 3) / 4;
    private:
        static const std::map<unsigned int, byte> m_bps_to_byte;
        static const std::map<byte, std::string> m_byte_to_bps;
        std::array<byte, size> m_data;

        static unsigned int chars_to_int(char c1, char c2, char c3, char c4);

    public:
        kmer();
        explicit kmer(std::array<byte, size> m_data);
        const std::array<byte, size>& data() const;
        std::string string() const;
        void print(std::ostream& ostream) const;
    };
}

#include "kmer.tpp"
