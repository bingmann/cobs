/*******************************************************************************
 * cobs/util/misc.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <array>
#include <cobs/util/misc.hpp>
#include <iostream>
#include <unistd.h>

#include <tlx/die.hpp>

namespace cobs {

uint64_t get_page_size() {
    int page_size = getpagesize();
    die_unless(page_size > 0);
    die_unless(page_size == 4096);     // todo check for experiments
    return (uint64_t)page_size;
}

std::string random_sequence(size_t size, size_t seed) {
    std::array<char, 4> basepairs = { 'A', 'C', 'G', 'T' };
    std::string result;
    std::srand(seed);
    for (size_t i = 0; i < size; i++) {
        result += basepairs[std::rand() % 4];
    }
    return result;
}

void initialize_map() {
//    std::array<char, 4> chars = {'A', 'C', 'G', 'T'};
//    int b = 0;
//    for (uint8_t i = 0; i < 4; i++) {
//        for (uint8_t j = 0; j < 4; j++) {
//            for (uint8_t k = 0; k < 4; k++) {
//                for (uint8_t o = 0; o < 4; o++) {
//                    char c[4] = {chars[i], chars[j], chars[k], chars[o]};
//                    std::cout << "{" << *((unsigned int*) c) << ", " << b++ << "}, " << std::endl << std::flush;
//                    std::cout << "{" << (unsigned int) b++ << ", \"" << chars[i] << chars[j] << chars[k] << chars[o] << "\"}, " << std::endl << std::flush;
//                    m_bps_to_uint8_t[chars_to_int(chars[i], chars[j], chars[k], chars[o])] = b++;
//                }
//            }
//        }
//    }
}

void initialize_map_server() {
    std::cout << "{";
    for (uint64_t i = 0; i < 16; i++) {
        uint64_t result = 0;
        result |= (i & 1);
        result |= (i & 2) << 15;
        result |= (i & 4) << 30;
        result |= (i & 8) << 45;
        std::cout << result << ", ";
    }
    std::cout << "}" << std::endl;
}

void deallocate_aligned(void* counts) {
    free(counts);
}

} // namespace cobs

/******************************************************************************/
