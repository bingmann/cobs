//
// Created by Florian Gauger on 08.09.2017.
//

#include "kmer.hpp"

//template<unsigned int N>
//int kmer<N>::chars_to_int(char c1, char c2, char c3, char c4) {
//    return (c1 << 24) + (c2 << 16) + (c3 << 8) + c4;
//}
//
//template<unsigned int N>
//void kmer<N>::initialize_map() {
//    std::array<char, 4> chars = {'A', 'C', 'G', 'T'};
//    for (unsigned char i = 0; i < 4; i++) {
//        for (unsigned char j = 0; j < 4; j++) {
//            for (unsigned char k = 0; k < 4; k++) {
//                for (unsigned char o = 0; o < 4; o++) {
//                    map[chars_to_int(chars[i], chars[j], chars[k], chars[o])] = (i << 6) + (j << 4) + (k << 2) + o;
//                }
//            }
//        }
//    }
//}
//
//template<unsigned int N>
//void kmer<N>::initialize_data(char* kmer_string) {
//    for (int i = 0; i < N; i += 4) {
//        m_data[i / 4] =  map[*((int*) kmer_string[i])];
//    }
//    if (N / 4 != 0) {
//        int i = N / 4 * 4;
//        char c2 = i++ >= N ? 'A' : kmer_string[i];
//        char c3 = i++ >= N ? 'A' : kmer_string[i];
//        char c4 = i++ >= N ? 'A' : kmer_string[i];
//        m_data[N / 4] = map[chars_to_int(kmer_string[i], c2, c3, c4)];
//    }
//}
//
////template<unsigned int N>
////kmer<N>::kmer() {
////    initialize_map();
////}
//
////template<unsigned int N>
////kmer<N>::kmer(char* kmer_string) : kmer() {
////    initialize_data(kmer_string);
////}
//
//template<unsigned int N>
//char* kmer<N>::data() {
//    return m_data.data();
//}
//
////template<unsigned int N>
////template<class Archive>
////void kmer<N>::serialize(Archive &ar, const unsigned int version) {
////    ar & size;
////    ar & m_data;
////}
