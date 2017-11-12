#pragma once

#include <array>
#include <string>
#include <map>
#include <ostream>
#include <util.hpp>

namespace isi {
    template<unsigned int N>
    class kmer {
    public:
        static const size_t size = (N + 3) / 4;
    private:
        static const std::map<unsigned int, uint8_t> m_bps_to_uint8_t;
        static const std::map<uint8_t, std::string> m_uint8_t_to_bps;
        std::array<uint8_t, size> m_data;

        static unsigned int chars_to_int(char c1, char c2, char c3, char c4);

    public:
        kmer();
        explicit kmer(std::array<uint8_t, size> m_data);
        void init(const char* chars);
        const std::array<uint8_t, size>& data() const;
        std::string string() const;
        void print(std::ostream& ostream) const;

        static void init(const char* chars, char* kmer, uint32_t kmer_size);
        static uint32_t data_size(uint32_t kmer_size) {
            return (kmer_size + 3) / 4;
        }
    };
}

#include "kmer.tpp"
