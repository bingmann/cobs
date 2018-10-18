/*******************************************************************************
 * src/main_avx.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>

int main() {
//    std::vector<char> data(32 * 10);
////    std::iota(data.begin(), data.end(), 0);
//    __m256i_u* d = reinterpret_cast<__m256i_u*>(data.data());
////    __m256i x = _mm256_loadu_si256(d);
////    __m256i y = _mm256_loadu_si256(d + 1);
////    *(d + 2) = _mm256_add_epi8(x, y);
//    std::array<uint8_t, 2> val = {7, 2};
//
//    __m256i_u mask = _mm256_set_epi8(1, -1, 5, -1, 9, -1, 13, -1, 17, -1, 21, -1, 25, -1, 29, -1, 0, -1, 4, -1, 8, -1, 12, -1, 16, -1, 20, -1, 24, -1, 28, -1);
//    __m256i_u mask2 = _mm256_set_epi8(128, -1, 64, -1, 32, -1, 16, -1, 8, -1, 4, -1, 2, -1, 1, -1, 128, -1, 64, -1, 32, -1, 16, -1, 8, -1, 4, -1, 2, -1, 1, -1);
//    __m256i_u mask3 = _mm256_set1_epi8(1);
//
//    *d = _mm256_set1_epi16(*reinterpret_cast<uint16_t*>(val.data()));
//    *(d + 1) = _mm256_shuffle_epi8(*d, mask);
//    *(d + 2) = _mm256_and_si256(*(d + 1), mask2);
//    *(d + 3) = _mm256_cmpeq_epi8(*(d + 2), mask2);
//    *(d + 4) = _mm256_and_si256(*(d + 3), mask3);
//    *(d + 5) = _mm256_add_epi16(*(d + 4), *(d + 4));
////    __m256i z = _mm256_shuffle_epi8(y,mask1);
////    z = _mm256_and_si256(z,mask2);
//
//    for (size_t i = 0; i < data.size(); i++) {
//        std::cout << std::setw(5) << (int) data[i];
//        if (i % 32 == 31) {
//            std::cout << std::endl;
//        }
//    }
//
//    for (size_t i = 0; i < 256; i++) {
//        for(int j = 0; j < 8; j++) {
//            std::cout << ((i / (int) std::pow(2, j)) % 2 == 1 ? 1 : 0) << ", ";
//        }
//        std::cout << std::endl;
//    }
}

/******************************************************************************/
