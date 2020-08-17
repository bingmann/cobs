/*******************************************************************************
 * cobs/kmer.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_KMER_HEADER
#define COBS_KMER_HEADER

#include <array>
#include <cassert>
#include <ostream>
#include <string>
#include <unordered_map>

#include <cobs/util/misc.hpp>
#include <cobs/util/query.hpp>

#include <tlx/define/likely.hpp>

namespace cobs {

static inline
uint32_t chars_to_int(char c1, char c2, char c3, char c4) {
    return uint32_t(0) | (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
}

extern const char* kmer_byte_to_base_pairs[256];
extern const std::unordered_map<unsigned int, uint8_t> kmer_bps_to_uint8_t;
extern uint8_t kmer_mirror_pairs[256];

template <unsigned int N>
class KMer : public std::array<uint8_t, (N + 3) / 4>
{
public:
    static const size_t size = (N + 3) / 4;

    using Super = std::array<uint8_t, (N + 3) / 4>;
    using Super::data;

public:
    KMer() {
        static_assert(sizeof(KMer<N>) == KMer<N>::size);
    }

    explicit KMer(const char* chars) {
        static_assert(sizeof(KMer<N>) == KMer<N>::size);
        init(chars);
    }

    void init(const char* chars) {
        for (int i = N - 4; i >= -3; i -= 4) {
            if (i >= 0) {
                data()[(N - (i + 4)) / 4] =
                    kmer_bps_to_uint8_t.at(*((uint32_t*)(chars + i)));
            }
            else {
                char c2 = i < -1 ? 'A' : chars[i + 1];
                char c3 = i < -2 ? 'A' : chars[i + 2];
                data()[size - 1] =
                    kmer_bps_to_uint8_t.at(chars_to_int(chars[i + 3], c3, c2, 'A'));
            }
        }
    }

    std::string string() const {
        std::string result;
        result.reserve(N);
        for (size_t i = 0; i < size; ++i) {
            if (TLX_UNLIKELY(i == 0 && N % 4 != 0)) {
                result += kmer_byte_to_base_pairs[data()[size - 1 - i]] + (4 - N % 4);
            }
            else {
                result += kmer_byte_to_base_pairs[data()[size - 1 - i]];
            }
        }
        return result;
    }

    std::string& to_string(std::string* out) const {
        out->clear();
        for (size_t i = 0; i < size; ++i) {
            if (TLX_UNLIKELY(i == 0 && N % 4 != 0)) {
                *out += kmer_byte_to_base_pairs[data()[size - 1 - i]] + (4 - N % 4);
            }
            else {
                *out += kmer_byte_to_base_pairs[data()[size - 1 - i]];
            }
        }
        return *out;
    }

    void print(std::ostream& ostream) const {
        ostream << string();
    }

    static void init(const char* chars, char* kmer_data, uint32_t kmer_size) {
        int kmer_size_int = kmer_size;
        for (int i = kmer_size_int - 4; i >= -3; i -= 4) {
            if (i >= 0) {
                kmer_data[(kmer_size_int - (i + 4)) / 4] =
                    kmer_bps_to_uint8_t.at(*((uint32_t*)(chars + i)));
            }
            else {
                char c2 = i < -1 ? 'A' : chars[i + 1];
                char c3 = i < -2 ? 'A' : chars[i + 2];
                kmer_data[data_size(kmer_size) - 1] =
                    kmer_bps_to_uint8_t.at(chars_to_int(chars[i + 3], c3, c2, 'A'));
            }
        }
    }

    static uint32_t data_size(uint32_t kmer_size) {
        return (kmer_size + 3) / 4;
    }

    bool operator < (const KMer& b) const {
        return std::lexicographical_compare(
            this->rbegin(), this->rend(), b.rbegin(), b.rend());
    }

    template <typename RandomGenerator>
    void fill_random(RandomGenerator& rng) {
        size_t i = 0;
        for ( ; i + 3 < size; i += 4) {
            *reinterpret_cast<uint32_t*>(data() + i) = rng();
        }
        for ( ; i < size; ++i) {
            data()[i] = rng();
        }
    }

    //! return 0 (A), 1 (C), 2 (G), or 3 (T) letter at index
    uint8_t at(size_t index) const {
        assert(index < N);
        // skip unused bits at the end
        index += (4 - N % 4);
        return (data()[size - 1 - index / 4] >> (6 - 2 * (index % 4))) & 0x3;
    }

    void canonicalize() {
        // base pair mirror_map map. A -> T, C -> G, G -> C, T -> A.
        // static const uint8_t mirror_map[4] = { 3, 2, 1, 0 };

        size_t i = 0, r = N - 1;
        while (at(i) == (3 - at(r)) && i < N / 2)
            ++i, --r;

        if (at(i) > (3 - at(r)))
            mirror();
    }

    void mirror() {
        // reverse kmer base pairs into a buffer and copy back
        std::array<uint8_t, size> buffer;

        // last byte contains only (N % 4) base pairs
        uint8_t overflow =
            static_cast<uint8_t>(data()[size - 1]) << (2 * (4 - N % 4));
        for (size_t i = 1; i < size; i++) {
            uint8_t bp = static_cast<uint8_t>(data()[size - 1 - i]);
            overflow |= bp >> (2 * (N % 4));
            buffer[i - 1] = kmer_mirror_pairs[overflow];
            overflow = bp << (2 * (4 - N % 4));
        }
        buffer[size - 1] = kmer_mirror_pairs[overflow];

        std::copy(buffer.begin(), buffer.end(), this->begin());
    }
};

} // namespace cobs

#endif // !COBS_KMER_HEADER

/******************************************************************************/
