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



template<unsigned int N>
unsigned int kmer<N>::chars_to_int(char c1, char c2, char c3, char c4) {
    unsigned int result = 0;
    return result + (c1 << 24) + (c2 << 16) + (c3 << 8) + c4;
}

template<unsigned int N>
void kmer<N>::initialize_map() {
//    std::array<char, 4> chars = {'A', 'C', 'G', 'T'};
//    byte b = 0;
//    for (byte i = 0; i < 4; i++) {
//        for (byte j = 0; j < 4; j++) {
//            for (byte k = 0; k < 4; k++) {
//                for (byte o = 0; o < 4; o++) {
//                    std::cout << "{" << (unsigned int) b++ << ", \"" << chars[o] << chars[k] << chars[j] << chars[i] << "\"}, " << std::endl << std::flush;
////                    m_bps_to_byte[chars_to_int(chars[i], chars[j], chars[k], chars[o])] = b++;
//                }
//            }
//        }
//    }
}

template<unsigned int N>
template <typename OutputIterator>
void kmer<N>::initialize_data(const char* kmer_string, OutputIterator outputIterator) {
    for (int i = 0; i < N; i += 4) {
        if (i < N - 3) {
            *outputIterator++ = m_bps_to_byte.at(*((unsigned int *) (kmer_string + i)));
        } else {
            i = N / 4 * 4;
            char c2 = i + 1 >= N ? 'A' : kmer_string[i + 1];
            char c3 = i + 2 >= N ? 'A' : kmer_string[i + 2];
            char c4 = i + 3 >= N ? 'A' : kmer_string[i + 3];
            *outputIterator++ = m_bps_to_byte.at(chars_to_int(c4, c3, c2, kmer_string[i]));
        }
    }
}

template<unsigned int N>
kmer<N>::kmer() {
    initialize_map();
}

template<unsigned int N>
kmer<N>::kmer(const char* kmer_string) : kmer() {
    initialize_data(kmer_string, m_data.begin());
}

template<unsigned int N>
byte* kmer<N>::data() {
    return m_data.data();
}

template<unsigned int N>
template<class Archive>
void kmer<N>::serialize(Archive &ar, const unsigned int version) {
    ar & m_data;
}

//template<unsigned int N>
//std::ostream& operator<<(std::ostream &os, const kmer<N>& km) {
//    std::transform(km.m_data.begin(), km.m_data.end(), std::ostream_iterator<std::ostream>(os), [&](byte b){
//        return kmer<N>::m_byte_to_bps.at(b);
//    });
//    return os;
//}

template<unsigned int N>
size_t kmer<N>::size() {
    return m_size;
}


template<unsigned int N>
template<typename Iterator>
kmer<N>::kmer(Iterator begin, Iterator end) {
    std::copy(begin, end, m_data.begin());
}

template<unsigned int N>
void kmer<N>::print(std::ostream &ostream) const {
//    std::transform(m_data.begin(), m_data.end(), std::ostream_iterator<std::ostream>(ostream), [&](byte b){
//        return kmer<N>::m_byte_to_bps.at(b);
//    });
    for(size_t i = 0; i < m_data.size(); i++) {
        ostream << m_byte_to_bps.at(m_data[i]);
    }
}

template<unsigned int N>
const std::map<unsigned int, byte> kmer<N>::m_bps_to_byte = {
    {1094795585, 0}, {1094795587, 1}, {1094795591, 2}, {1094795604, 3}, {1094796097, 4},
    {1094796099, 5}, {1094796103, 6}, {1094796116, 7}, {1094797121, 8}, {1094797123, 9},
    {1094797127, 10}, {1094797140, 11}, {1094800449, 12}, {1094800451, 13}, {1094800455, 14},
    {1094800468, 15}, {1094926657, 16}, {1094926659, 17}, {1094926663, 18}, {1094926676, 19},
    {1094927169, 20}, {1094927171, 21}, {1094927175, 22}, {1094927188, 23}, {1094928193, 24},
    {1094928195, 25}, {1094928199, 26}, {1094928212, 27}, {1094931521, 28}, {1094931523, 29},
    {1094931527, 30}, {1094931540, 31}, {1095188801, 32}, {1095188803, 33}, {1095188807, 34},
    {1095188820, 35}, {1095189313, 36}, {1095189315, 37}, {1095189319, 38}, {1095189332, 39},
    {1095190337, 40}, {1095190339, 41}, {1095190343, 42}, {1095190356, 43}, {1095193665, 44},
    {1095193667, 45}, {1095193671, 46}, {1095193684, 47}, {1096040769, 48}, {1096040771, 49},
    {1096040775, 50}, {1096040788, 51}, {1096041281, 52}, {1096041283, 53}, {1096041287, 54},
    {1096041300, 55}, {1096042305, 56}, {1096042307, 57}, {1096042311, 58}, {1096042324, 59},
    {1096045633, 60}, {1096045635, 61}, {1096045639, 62}, {1096045652, 63}, {1128350017, 64},
    {1128350019, 65}, {1128350023, 66}, {1128350036, 67}, {1128350529, 68}, {1128350531, 69},
    {1128350535, 70}, {1128350548, 71}, {1128351553, 72}, {1128351555, 73}, {1128351559, 74},
    {1128351572, 75}, {1128354881, 76}, {1128354883, 77}, {1128354887, 78}, {1128354900, 79},
    {1128481089, 80}, {1128481091, 81}, {1128481095, 82}, {1128481108, 83}, {1128481601, 84},
    {1128481603, 85}, {1128481607, 86}, {1128481620, 87}, {1128482625, 88}, {1128482627, 89},
    {1128482631, 90}, {1128482644, 91}, {1128485953, 92}, {1128485955, 93}, {1128485959, 94},
    {1128485972, 95}, {1128743233, 96}, {1128743235, 97}, {1128743239, 98}, {1128743252, 99},
    {1128743745, 100}, {1128743747, 101}, {1128743751, 102}, {1128743764, 103}, {1128744769, 104},
    {1128744771, 105}, {1128744775, 106}, {1128744788, 107}, {1128748097, 108}, {1128748099, 109},
    {1128748103, 110}, {1128748116, 111}, {1129595201, 112}, {1129595203, 113}, {1129595207, 114},
    {1129595220, 115}, {1129595713, 116}, {1129595715, 117}, {1129595719, 118}, {1129595732, 119},
    {1129596737, 120}, {1129596739, 121}, {1129596743, 122}, {1129596756, 123}, {1129600065, 124},
    {1129600067, 125}, {1129600071, 126}, {1129600084, 127}, {1195458881, 128}, {1195458883, 129},
    {1195458887, 130}, {1195458900, 131}, {1195459393, 132}, {1195459395, 133}, {1195459399, 134},
    {1195459412, 135}, {1195460417, 136}, {1195460419, 137}, {1195460423, 138}, {1195460436, 139},
    {1195463745, 140}, {1195463747, 141}, {1195463751, 142}, {1195463764, 143}, {1195589953, 144},
    {1195589955, 145}, {1195589959, 146}, {1195589972, 147}, {1195590465, 148}, {1195590467, 149},
    {1195590471, 150}, {1195590484, 151}, {1195591489, 152}, {1195591491, 153}, {1195591495, 154},
    {1195591508, 155}, {1195594817, 156}, {1195594819, 157}, {1195594823, 158}, {1195594836, 159},
    {1195852097, 160}, {1195852099, 161}, {1195852103, 162}, {1195852116, 163}, {1195852609, 164},
    {1195852611, 165}, {1195852615, 166}, {1195852628, 167}, {1195853633, 168}, {1195853635, 169},
    {1195853639, 170}, {1195853652, 171}, {1195856961, 172}, {1195856963, 173}, {1195856967, 174},
    {1195856980, 175}, {1196704065, 176}, {1196704067, 177}, {1196704071, 178}, {1196704084, 179},
    {1196704577, 180}, {1196704579, 181}, {1196704583, 182}, {1196704596, 183}, {1196705601, 184},
    {1196705603, 185}, {1196705607, 186}, {1196705620, 187}, {1196708929, 188}, {1196708931, 189},
    {1196708935, 190}, {1196708948, 191}, {1413562689, 192}, {1413562691, 193}, {1413562695, 194},
    {1413562708, 195}, {1413563201, 196}, {1413563203, 197}, {1413563207, 198}, {1413563220, 199},
    {1413564225, 200}, {1413564227, 201}, {1413564231, 202}, {1413564244, 203}, {1413567553, 204},
    {1413567555, 205}, {1413567559, 206}, {1413567572, 207}, {1413693761, 208}, {1413693763, 209},
    {1413693767, 210}, {1413693780, 211}, {1413694273, 212}, {1413694275, 213}, {1413694279, 214},
    {1413694292, 215}, {1413695297, 216}, {1413695299, 217}, {1413695303, 218}, {1413695316, 219},
    {1413698625, 220}, {1413698627, 221}, {1413698631, 222}, {1413698644, 223}, {1413955905, 224},
    {1413955907, 225}, {1413955911, 226}, {1413955924, 227}, {1413956417, 228}, {1413956419, 229},
    {1413956423, 230}, {1413956436, 231}, {1413957441, 232}, {1413957443, 233}, {1413957447, 234},
    {1413957460, 235}, {1413960769, 236}, {1413960771, 237}, {1413960775, 238}, {1413960788, 239},
    {1414807873, 240}, {1414807875, 241}, {1414807879, 242}, {1414807892, 243}, {1414808385, 244},
    {1414808387, 245}, {1414808391, 246}, {1414808404, 247}, {1414809409, 248}, {1414809411, 249},
    {1414809415, 250}, {1414809428, 251}, {1414812737, 252}, {1414812739, 253}, {1414812743, 254},
    {1414812756, 255}};

template<unsigned int N>
const std::map<byte, std::string> kmer<N>::m_byte_to_bps = {
        {0, "AAAA"}, {1, "CAAA"}, {2, "GAAA"}, {3, "TAAA"}, {4, "ACAA"}, {5, "CCAA"}, {6, "GCAA"}, {7, "TCAA"},
        {8, "AGAA"}, {9, "CGAA"}, {10, "GGAA"}, {11, "TGAA"}, {12, "ATAA"}, {13, "CTAA"}, {14, "GTAA"}, {15, "TTAA"},
        {16, "AACA"}, {17, "CACA"}, {18, "GACA"}, {19, "TACA"}, {20, "ACCA"}, {21, "CCCA"}, {22, "GCCA"}, {23, "TCCA"},
        {24, "AGCA"}, {25, "CGCA"}, {26, "GGCA"}, {27, "TGCA"}, {28, "ATCA"}, {29, "CTCA"}, {30, "GTCA"}, {31, "TTCA"},
        {32, "AAGA"}, {33, "CAGA"}, {34, "GAGA"}, {35, "TAGA"}, {36, "ACGA"}, {37, "CCGA"}, {38, "GCGA"}, {39, "TCGA"},
        {40, "AGGA"}, {41, "CGGA"}, {42, "GGGA"}, {43, "TGGA"}, {44, "ATGA"}, {45, "CTGA"}, {46, "GTGA"}, {47, "TTGA"},
        {48, "AATA"}, {49, "CATA"}, {50, "GATA"}, {51, "TATA"}, {52, "ACTA"}, {53, "CCTA"}, {54, "GCTA"}, {55, "TCTA"},
        {56, "AGTA"}, {57, "CGTA"}, {58, "GGTA"}, {59, "TGTA"}, {60, "ATTA"}, {61, "CTTA"}, {62, "GTTA"}, {63, "TTTA"},
        {64, "AAAC"}, {65, "CAAC"}, {66, "GAAC"}, {67, "TAAC"}, {68, "ACAC"}, {69, "CCAC"}, {70, "GCAC"}, {71, "TCAC"},
        {72, "AGAC"}, {73, "CGAC"}, {74, "GGAC"}, {75, "TGAC"}, {76, "ATAC"}, {77, "CTAC"}, {78, "GTAC"}, {79, "TTAC"},
        {80, "AACC"}, {81, "CACC"}, {82, "GACC"}, {83, "TACC"}, {84, "ACCC"}, {85, "CCCC"}, {86, "GCCC"}, {87, "TCCC"},
        {88, "AGCC"}, {89, "CGCC"}, {90, "GGCC"}, {91, "TGCC"}, {92, "ATCC"}, {93, "CTCC"}, {94, "GTCC"}, {95, "TTCC"},
        {96, "AAGC"}, {97, "CAGC"}, {98, "GAGC"}, {99, "TAGC"}, {100, "ACGC"}, {101, "CCGC"}, {102, "GCGC"}, {103, "TCGC"},
        {104, "AGGC"}, {105, "CGGC"}, {106, "GGGC"}, {107, "TGGC"}, {108, "ATGC"}, {109, "CTGC"}, {110, "GTGC"}, {111, "TTGC"},
        {112, "AATC"}, {113, "CATC"}, {114, "GATC"}, {115, "TATC"}, {116, "ACTC"}, {117, "CCTC"}, {118, "GCTC"}, {119, "TCTC"},
        {120, "AGTC"}, {121, "CGTC"}, {122, "GGTC"}, {123, "TGTC"}, {124, "ATTC"}, {125, "CTTC"}, {126, "GTTC"}, {127, "TTTC"},
        {128, "AAAG"}, {129, "CAAG"}, {130, "GAAG"}, {131, "TAAG"}, {132, "ACAG"}, {133, "CCAG"}, {134, "GCAG"}, {135, "TCAG"},
        {136, "AGAG"}, {137, "CGAG"}, {138, "GGAG"}, {139, "TGAG"}, {140, "ATAG"}, {141, "CTAG"}, {142, "GTAG"}, {143, "TTAG"},
        {144, "AACG"}, {145, "CACG"}, {146, "GACG"}, {147, "TACG"}, {148, "ACCG"}, {149, "CCCG"}, {150, "GCCG"}, {151, "TCCG"},
        {152, "AGCG"}, {153, "CGCG"}, {154, "GGCG"}, {155, "TGCG"}, {156, "ATCG"}, {157, "CTCG"}, {158, "GTCG"}, {159, "TTCG"},
        {160, "AAGG"}, {161, "CAGG"}, {162, "GAGG"}, {163, "TAGG"}, {164, "ACGG"}, {165, "CCGG"}, {166, "GCGG"}, {167, "TCGG"},
        {168, "AGGG"}, {169, "CGGG"}, {170, "GGGG"}, {171, "TGGG"}, {172, "ATGG"}, {173, "CTGG"}, {174, "GTGG"}, {175, "TTGG"},
        {176, "AATG"}, {177, "CATG"}, {178, "GATG"}, {179, "TATG"}, {180, "ACTG"}, {181, "CCTG"}, {182, "GCTG"}, {183, "TCTG"},
        {184, "AGTG"}, {185, "CGTG"}, {186, "GGTG"}, {187, "TGTG"}, {188, "ATTG"}, {189, "CTTG"}, {190, "GTTG"}, {191, "TTTG"},
        {192, "AAAT"}, {193, "CAAT"}, {194, "GAAT"}, {195, "TAAT"}, {196, "ACAT"}, {197, "CCAT"}, {198, "GCAT"}, {199, "TCAT"},
        {200, "AGAT"}, {201, "CGAT"}, {202, "GGAT"}, {203, "TGAT"}, {204, "ATAT"}, {205, "CTAT"}, {206, "GTAT"}, {207, "TTAT"},
        {208, "AACT"}, {209, "CACT"}, {210, "GACT"}, {211, "TACT"}, {212, "ACCT"}, {213, "CCCT"}, {214, "GCCT"}, {215, "TCCT"},
        {216, "AGCT"}, {217, "CGCT"}, {218, "GGCT"}, {219, "TGCT"}, {220, "ATCT"}, {221, "CTCT"}, {222, "GTCT"}, {223, "TTCT"},
        {224, "AAGT"}, {225, "CAGT"}, {226, "GAGT"}, {227, "TAGT"}, {228, "ACGT"}, {229, "CCGT"}, {230, "GCGT"}, {231, "TCGT"},
        {232, "AGGT"}, {233, "CGGT"}, {234, "GGGT"}, {235, "TGGT"}, {236, "ATGT"}, {237, "CTGT"}, {238, "GTGT"}, {239, "TTGT"},
        {240, "AATT"}, {241, "CATT"}, {242, "GATT"}, {243, "TATT"}, {244, "ACTT"}, {245, "CCTT"}, {246, "GCTT"}, {247, "TCTT"},
        {248, "AGTT"}, {249, "CGTT"}, {250, "GGTT"}, {251, "TGTT"}, {252, "ATTT"}, {253, "CTTT"}, {254, "GTTT"}, {255, "TTTT"}
};

