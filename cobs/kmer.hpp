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

template <unsigned int N>
class kmer : public std::array<uint8_t, (N + 3) / 4>
{
public:
    static const size_t size = (N + 3) / 4;

    using Super = std::array<uint8_t, (N + 3) / 4>;
    using Super::data;

private:
    static const std::unordered_map<unsigned int, uint8_t> bps_to_uint8_t_;
    static const char* to_base_pairs_[256];
    static uint8_t mirror_pairs_[256];

public:
    kmer() {
        static_assert(sizeof(kmer<N>) == kmer<N>::size);
    }

    explicit kmer(const char* chars) {
        static_assert(sizeof(kmer<N>) == kmer<N>::size);
        init(chars);
    }

    void init(const char* chars) {
        for (int i = N - 4; i >= -3; i -= 4) {
            if (i >= 0) {
                data()[(N - (i + 4)) / 4] =
                    bps_to_uint8_t_.at(*((uint32_t*)(chars + i)));
            }
            else {
                char c2 = i < -1 ? 'A' : chars[i + 1];
                char c3 = i < -2 ? 'A' : chars[i + 2];
                data()[size - 1] =
                    bps_to_uint8_t_.at(chars_to_int(chars[i + 3], c3, c2, 'A'));
            }
        }
    }

    std::string string() const {
        std::string result;
        result.reserve(N);
        for (size_t i = 0; i < size; ++i) {
            if (TLX_UNLIKELY(i == 0 && N % 4 != 0)) {
                result += to_base_pairs_[data()[size - 1 - i]] + (4 - N % 4);
            }
            else {
                result += to_base_pairs_[data()[size - 1 - i]];
            }
        }
        return result;
    }

    void print(std::ostream& ostream) const {
        ostream << string();
    }

    static void init(const char* chars, char* kmer_data, uint32_t kmer_size) {
        int kmer_size_int = kmer_size;
        for (int i = kmer_size_int - 4; i >= -3; i -= 4) {
            if (i >= 0) {
                kmer_data[(kmer_size_int - (i + 4)) / 4] =
                    bps_to_uint8_t_.at(*((uint32_t*)(chars + i)));
            }
            else {
                char c2 = i < -1 ? 'A' : chars[i + 1];
                char c3 = i < -2 ? 'A' : chars[i + 2];
                kmer_data[data_size(kmer_size) - 1] =
                    bps_to_uint8_t_.at(chars_to_int(chars[i + 3], c3, c2, 'A'));
            }
        }
    }

    static uint32_t data_size(uint32_t kmer_size) {
        return (kmer_size + 3) / 4;
    }

    bool operator < (const kmer& b) const {
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
            buffer[i - 1] = mirror_pairs_[overflow];
            overflow = bp << (2 * (4 - N % 4));
        }
        buffer[size - 1] = mirror_pairs_[overflow];

        std::copy(buffer.begin(), buffer.end(), this->begin());
    }
};

template <unsigned int N>
const std::unordered_map<unsigned int, uint8_t> kmer<N>::bps_to_uint8_t_ = {
    { 1094795585, 0 }, { 1128350017, 1 }, { 1195458881, 2 },
    { 1413562689, 3 }, { 1094926657, 4 }, { 1128481089, 5 },
    { 1195589953, 6 }, { 1413693761, 7 }, { 1095188801, 8 },
    { 1128743233, 9 }, { 1195852097, 10 }, { 1413955905, 11 },
    { 1096040769, 12 }, { 1129595201, 13 }, { 1196704065, 14 },
    { 1414807873, 15 }, { 1094796097, 16 }, { 1128350529, 17 },
    { 1195459393, 18 }, { 1413563201, 19 }, { 1094927169, 20 },
    { 1128481601, 21 }, { 1195590465, 22 }, { 1413694273, 23 },
    { 1095189313, 24 }, { 1128743745, 25 }, { 1195852609, 26 },
    { 1413956417, 27 }, { 1096041281, 28 }, { 1129595713, 29 },
    { 1196704577, 30 }, { 1414808385, 31 }, { 1094797121, 32 },
    { 1128351553, 33 }, { 1195460417, 34 }, { 1413564225, 35 },
    { 1094928193, 36 }, { 1128482625, 37 }, { 1195591489, 38 },
    { 1413695297, 39 }, { 1095190337, 40 }, { 1128744769, 41 },
    { 1195853633, 42 }, { 1413957441, 43 }, { 1096042305, 44 },
    { 1129596737, 45 }, { 1196705601, 46 }, { 1414809409, 47 },
    { 1094800449, 48 }, { 1128354881, 49 }, { 1195463745, 50 },
    { 1413567553, 51 }, { 1094931521, 52 }, { 1128485953, 53 },
    { 1195594817, 54 }, { 1413698625, 55 }, { 1095193665, 56 },
    { 1128748097, 57 }, { 1195856961, 58 }, { 1413960769, 59 },
    { 1096045633, 60 }, { 1129600065, 61 }, { 1196708929, 62 },
    { 1414812737, 63 }, { 1094795587, 64 }, { 1128350019, 65 },
    { 1195458883, 66 }, { 1413562691, 67 }, { 1094926659, 68 },
    { 1128481091, 69 }, { 1195589955, 70 }, { 1413693763, 71 },
    { 1095188803, 72 }, { 1128743235, 73 }, { 1195852099, 74 },
    { 1413955907, 75 }, { 1096040771, 76 }, { 1129595203, 77 },
    { 1196704067, 78 }, { 1414807875, 79 }, { 1094796099, 80 },
    { 1128350531, 81 }, { 1195459395, 82 }, { 1413563203, 83 },
    { 1094927171, 84 }, { 1128481603, 85 }, { 1195590467, 86 },
    { 1413694275, 87 }, { 1095189315, 88 }, { 1128743747, 89 },
    { 1195852611, 90 }, { 1413956419, 91 }, { 1096041283, 92 },
    { 1129595715, 93 }, { 1196704579, 94 }, { 1414808387, 95 },
    { 1094797123, 96 }, { 1128351555, 97 }, { 1195460419, 98 },
    { 1413564227, 99 }, { 1094928195, 100 }, { 1128482627, 101 },
    { 1195591491, 102 }, { 1413695299, 103 }, { 1095190339, 104 },
    { 1128744771, 105 }, { 1195853635, 106 }, { 1413957443, 107 },
    { 1096042307, 108 }, { 1129596739, 109 }, { 1196705603, 110 },
    { 1414809411, 111 }, { 1094800451, 112 }, { 1128354883, 113 },
    { 1195463747, 114 }, { 1413567555, 115 }, { 1094931523, 116 },
    { 1128485955, 117 }, { 1195594819, 118 }, { 1413698627, 119 },
    { 1095193667, 120 }, { 1128748099, 121 }, { 1195856963, 122 },
    { 1413960771, 123 }, { 1096045635, 124 }, { 1129600067, 125 },
    { 1196708931, 126 }, { 1414812739, 127 }, { 1094795591, 128 },
    { 1128350023, 129 }, { 1195458887, 130 }, { 1413562695, 131 },
    { 1094926663, 132 }, { 1128481095, 133 }, { 1195589959, 134 },
    { 1413693767, 135 }, { 1095188807, 136 }, { 1128743239, 137 },
    { 1195852103, 138 }, { 1413955911, 139 }, { 1096040775, 140 },
    { 1129595207, 141 }, { 1196704071, 142 }, { 1414807879, 143 },
    { 1094796103, 144 }, { 1128350535, 145 }, { 1195459399, 146 },
    { 1413563207, 147 }, { 1094927175, 148 }, { 1128481607, 149 },
    { 1195590471, 150 }, { 1413694279, 151 }, { 1095189319, 152 },
    { 1128743751, 153 }, { 1195852615, 154 }, { 1413956423, 155 },
    { 1096041287, 156 }, { 1129595719, 157 }, { 1196704583, 158 },
    { 1414808391, 159 }, { 1094797127, 160 }, { 1128351559, 161 },
    { 1195460423, 162 }, { 1413564231, 163 }, { 1094928199, 164 },
    { 1128482631, 165 }, { 1195591495, 166 }, { 1413695303, 167 },
    { 1095190343, 168 }, { 1128744775, 169 }, { 1195853639, 170 },
    { 1413957447, 171 }, { 1096042311, 172 }, { 1129596743, 173 },
    { 1196705607, 174 }, { 1414809415, 175 }, { 1094800455, 176 },
    { 1128354887, 177 }, { 1195463751, 178 }, { 1413567559, 179 },
    { 1094931527, 180 }, { 1128485959, 181 }, { 1195594823, 182 },
    { 1413698631, 183 }, { 1095193671, 184 }, { 1128748103, 185 },
    { 1195856967, 186 }, { 1413960775, 187 }, { 1096045639, 188 },
    { 1129600071, 189 }, { 1196708935, 190 }, { 1414812743, 191 },
    { 1094795604, 192 }, { 1128350036, 193 }, { 1195458900, 194 },
    { 1413562708, 195 }, { 1094926676, 196 }, { 1128481108, 197 },
    { 1195589972, 198 }, { 1413693780, 199 }, { 1095188820, 200 },
    { 1128743252, 201 }, { 1195852116, 202 }, { 1413955924, 203 },
    { 1096040788, 204 }, { 1129595220, 205 }, { 1196704084, 206 },
    { 1414807892, 207 }, { 1094796116, 208 }, { 1128350548, 209 },
    { 1195459412, 210 }, { 1413563220, 211 }, { 1094927188, 212 },
    { 1128481620, 213 }, { 1195590484, 214 }, { 1413694292, 215 },
    { 1095189332, 216 }, { 1128743764, 217 }, { 1195852628, 218 },
    { 1413956436, 219 }, { 1096041300, 220 }, { 1129595732, 221 },
    { 1196704596, 222 }, { 1414808404, 223 }, { 1094797140, 224 },
    { 1128351572, 225 }, { 1195460436, 226 }, { 1413564244, 227 },
    { 1094928212, 228 }, { 1128482644, 229 }, { 1195591508, 230 },
    { 1413695316, 231 }, { 1095190356, 232 }, { 1128744788, 233 },
    { 1195853652, 234 }, { 1413957460, 235 }, { 1096042324, 236 },
    { 1129596756, 237 }, { 1196705620, 238 }, { 1414809428, 239 },
    { 1094800468, 240 }, { 1128354900, 241 }, { 1195463764, 242 },
    { 1413567572, 243 }, { 1094931540, 244 }, { 1128485972, 245 },
    { 1195594836, 246 }, { 1413698644, 247 }, { 1095193684, 248 },
    { 1128748116, 249 }, { 1195856980, 250 }, { 1413960788, 251 },
    { 1096045652, 252 }, { 1129600084, 253 }, { 1196708948, 254 },
    { 1414812756, 255 }
};

template <unsigned int N>
const char* kmer<N>::to_base_pairs_[256] = {
    "AAAA", "AAAC", "AAAG", "AAAT", "AACA", "AACC", "AACG", "AACT",
    "AAGA", "AAGC", "AAGG", "AAGT", "AATA", "AATC", "AATG", "AATT",
    "ACAA", "ACAC", "ACAG", "ACAT", "ACCA", "ACCC", "ACCG", "ACCT",
    "ACGA", "ACGC", "ACGG", "ACGT", "ACTA", "ACTC", "ACTG", "ACTT",
    "AGAA", "AGAC", "AGAG", "AGAT", "AGCA", "AGCC", "AGCG", "AGCT",
    "AGGA", "AGGC", "AGGG", "AGGT", "AGTA", "AGTC", "AGTG", "AGTT",
    "ATAA", "ATAC", "ATAG", "ATAT", "ATCA", "ATCC", "ATCG", "ATCT",
    "ATGA", "ATGC", "ATGG", "ATGT", "ATTA", "ATTC", "ATTG", "ATTT",
    "CAAA", "CAAC", "CAAG", "CAAT", "CACA", "CACC", "CACG", "CACT",
    "CAGA", "CAGC", "CAGG", "CAGT", "CATA", "CATC", "CATG", "CATT",
    "CCAA", "CCAC", "CCAG", "CCAT", "CCCA", "CCCC", "CCCG", "CCCT",
    "CCGA", "CCGC", "CCGG", "CCGT", "CCTA", "CCTC", "CCTG", "CCTT",
    "CGAA", "CGAC", "CGAG", "CGAT", "CGCA", "CGCC", "CGCG", "CGCT",
    "CGGA", "CGGC", "CGGG", "CGGT", "CGTA", "CGTC", "CGTG", "CGTT",
    "CTAA", "CTAC", "CTAG", "CTAT", "CTCA", "CTCC", "CTCG", "CTCT",
    "CTGA", "CTGC", "CTGG", "CTGT", "CTTA", "CTTC", "CTTG", "CTTT",
    "GAAA", "GAAC", "GAAG", "GAAT", "GACA", "GACC", "GACG", "GACT",
    "GAGA", "GAGC", "GAGG", "GAGT", "GATA", "GATC", "GATG", "GATT",
    "GCAA", "GCAC", "GCAG", "GCAT", "GCCA", "GCCC", "GCCG", "GCCT",
    "GCGA", "GCGC", "GCGG", "GCGT", "GCTA", "GCTC", "GCTG", "GCTT",
    "GGAA", "GGAC", "GGAG", "GGAT", "GGCA", "GGCC", "GGCG", "GGCT",
    "GGGA", "GGGC", "GGGG", "GGGT", "GGTA", "GGTC", "GGTG", "GGTT",
    "GTAA", "GTAC", "GTAG", "GTAT", "GTCA", "GTCC", "GTCG", "GTCT",
    "GTGA", "GTGC", "GTGG", "GTGT", "GTTA", "GTTC", "GTTG", "GTTT",
    "TAAA", "TAAC", "TAAG", "TAAT", "TACA", "TACC", "TACG", "TACT",
    "TAGA", "TAGC", "TAGG", "TAGT", "TATA", "TATC", "TATG", "TATT",
    "TCAA", "TCAC", "TCAG", "TCAT", "TCCA", "TCCC", "TCCG", "TCCT",
    "TCGA", "TCGC", "TCGG", "TCGT", "TCTA", "TCTC", "TCTG", "TCTT",
    "TGAA", "TGAC", "TGAG", "TGAT", "TGCA", "TGCC", "TGCG", "TGCT",
    "TGGA", "TGGC", "TGGG", "TGGT", "TGTA", "TGTC", "TGTG", "TGTT",
    "TTAA", "TTAC", "TTAG", "TTAT", "TTCA", "TTCC", "TTCG", "TTCT",
    "TTGA", "TTGC", "TTGG", "TTGT", "TTTA", "TTTC", "TTTG", "TTTT"
};

/*
    for (size_t i = 0; i < 256; ++i) {
        size_t j =
            ((3 - ((i >> 0) & 3)) << 6) |
            ((3 - ((i >> 2) & 3)) << 4) |
            ((3 - ((i >> 4) & 3)) << 2) |
            ((3 - ((i >> 6) & 3)) << 0);
        printf("0x%02X, ", j);
        if (i % 8 == 7) printf("\n");
    }
 */
template <unsigned int N>
uint8_t kmer<N>::mirror_pairs_[256] = {
    0xFF, 0xBF, 0x7F, 0x3F, 0xEF, 0xAF, 0x6F, 0x2F,
    0xDF, 0x9F, 0x5F, 0x1F, 0xCF, 0x8F, 0x4F, 0x0F,
    0xFB, 0xBB, 0x7B, 0x3B, 0xEB, 0xAB, 0x6B, 0x2B,
    0xDB, 0x9B, 0x5B, 0x1B, 0xCB, 0x8B, 0x4B, 0x0B,
    0xF7, 0xB7, 0x77, 0x37, 0xE7, 0xA7, 0x67, 0x27,
    0xD7, 0x97, 0x57, 0x17, 0xC7, 0x87, 0x47, 0x07,
    0xF3, 0xB3, 0x73, 0x33, 0xE3, 0xA3, 0x63, 0x23,
    0xD3, 0x93, 0x53, 0x13, 0xC3, 0x83, 0x43, 0x03,
    0xFE, 0xBE, 0x7E, 0x3E, 0xEE, 0xAE, 0x6E, 0x2E,
    0xDE, 0x9E, 0x5E, 0x1E, 0xCE, 0x8E, 0x4E, 0x0E,
    0xFA, 0xBA, 0x7A, 0x3A, 0xEA, 0xAA, 0x6A, 0x2A,
    0xDA, 0x9A, 0x5A, 0x1A, 0xCA, 0x8A, 0x4A, 0x0A,
    0xF6, 0xB6, 0x76, 0x36, 0xE6, 0xA6, 0x66, 0x26,
    0xD6, 0x96, 0x56, 0x16, 0xC6, 0x86, 0x46, 0x06,
    0xF2, 0xB2, 0x72, 0x32, 0xE2, 0xA2, 0x62, 0x22,
    0xD2, 0x92, 0x52, 0x12, 0xC2, 0x82, 0x42, 0x02,
    0xFD, 0xBD, 0x7D, 0x3D, 0xED, 0xAD, 0x6D, 0x2D,
    0xDD, 0x9D, 0x5D, 0x1D, 0xCD, 0x8D, 0x4D, 0x0D,
    0xF9, 0xB9, 0x79, 0x39, 0xE9, 0xA9, 0x69, 0x29,
    0xD9, 0x99, 0x59, 0x19, 0xC9, 0x89, 0x49, 0x09,
    0xF5, 0xB5, 0x75, 0x35, 0xE5, 0xA5, 0x65, 0x25,
    0xD5, 0x95, 0x55, 0x15, 0xC5, 0x85, 0x45, 0x05,
    0xF1, 0xB1, 0x71, 0x31, 0xE1, 0xA1, 0x61, 0x21,
    0xD1, 0x91, 0x51, 0x11, 0xC1, 0x81, 0x41, 0x01,
    0xFC, 0xBC, 0x7C, 0x3C, 0xEC, 0xAC, 0x6C, 0x2C,
    0xDC, 0x9C, 0x5C, 0x1C, 0xCC, 0x8C, 0x4C, 0x0C,
    0xF8, 0xB8, 0x78, 0x38, 0xE8, 0xA8, 0x68, 0x28,
    0xD8, 0x98, 0x58, 0x18, 0xC8, 0x88, 0x48, 0x08,
    0xF4, 0xB4, 0x74, 0x34, 0xE4, 0xA4, 0x64, 0x24,
    0xD4, 0x94, 0x54, 0x14, 0xC4, 0x84, 0x44, 0x04,
    0xF0, 0xB0, 0x70, 0x30, 0xE0, 0xA0, 0x60, 0x20,
    0xD0, 0x90, 0x50, 0x10, 0xC0, 0x80, 0x40, 0x00
};

} // namespace cobs

#endif // !COBS_KMER_HEADER

/******************************************************************************/
