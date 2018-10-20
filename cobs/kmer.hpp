/*******************************************************************************
 * cobs/kmer.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_KMER_HEADER
#define COBS_KMER_HEADER
#pragma once

#include <array>
#include <ostream>
#include <string>
#include <unordered_map>

#include <cobs/util/misc.hpp>

namespace cobs {

static inline
unsigned int chars_to_int(char c1, char c2, char c3, char c4) {
    unsigned int result = 0;
    return result + (c1 << 24) + (c2 << 16) + (c3 << 8) + c4;
}

template <unsigned int N>
class kmer : public std::array<uint8_t, (N + 3) / 4>
{
public:
    static const size_t size = (N + 3) / 4;

    using Super = std::array<uint8_t, (N + 3) / 4>;
    using Super::data;

private:
    static const std::unordered_map<unsigned int, uint8_t> m_bps_to_uint8_t;
    static const char* m_to_base_pairs[256];

public:
    kmer() {
        static_assert(sizeof(kmer<N>) == kmer<N>::size);
    }

    void init(const char* chars) {
        for (int i = N - 4; i >= -3; i -= 4) {
            if (i >= 0) {
                data()[(N - (i + 4)) / 4] =
                    m_bps_to_uint8_t.at(*((uint32_t*)(chars + i)));
            }
            else {
                char c2 = i < -1 ? 'A' : chars[i + 1];
                char c3 = i < -2 ? 'A' : chars[i + 2];
                data()[size - 1] =
                    m_bps_to_uint8_t.at(chars_to_int(chars[i + 3], c3, c2, 'A'));
            }
        }
    }

    std::string string() const {
        std::string result;
        for (size_t i = 0; i < size; i++) {
            std::string uint8_t_string =
                m_to_base_pairs[data()[size - i - 1]];
            if (i == 0 && N % 4 != 0) {
                uint8_t_string =
                    uint8_t_string.substr(4 - N % 4, std::string::npos);
            }
            result += uint8_t_string;
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
                    m_bps_to_uint8_t.at(*((uint32_t*)(chars + i)));
            }
            else {
                char c2 = i < -1 ? 'A' : chars[i + 1];
                char c3 = i < -2 ? 'A' : chars[i + 2];
                kmer_data[data_size(kmer_size) - 1] =
                    m_bps_to_uint8_t.at(chars_to_int(chars[i + 3], c3, c2, 'A'));
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
};

template <unsigned int N>
const std::unordered_map<unsigned int, uint8_t> kmer<N>::m_bps_to_uint8_t = {
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
const char* kmer<N>::m_to_base_pairs[256] = {
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

} // namespace cobs

#endif // !COBS_KMER_HEADER

/******************************************************************************/
