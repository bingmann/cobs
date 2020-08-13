/*******************************************************************************
 * tests/util.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/misc.hpp>
#include <cobs/util/query.hpp>
#include <gtest/gtest.h>
#include <stdint.h>

void is_aligned(void* ptr, size_t alignment) {
    ASSERT_EQ((uintptr_t)ptr % alignment, 0);
}

TEST(util, allocate_aligned) {
    auto ptr1 = cobs::allocate_aligned<uint8_t>(10, cobs::get_page_size());
    is_aligned(ptr1, cobs::get_page_size());
    auto ptr2 = cobs::allocate_aligned<uint16_t>(1337, 16);
    is_aligned(ptr2, 16);
    cobs::deallocate_aligned(ptr1);
    cobs::deallocate_aligned(ptr2);
}

void test_kmer(const char* kmer_data,
               const char* kmer_correct, bool is_good) {
    char kmer_buffer[31];
    bool good = cobs::canonicalize_kmer(kmer_data, kmer_buffer, 31);

    die_unequal(std::string(kmer_buffer, 31),
                std::string(kmer_correct, 31));
    die_unequal(good, is_good);
}

TEST(util, kmer_canonicalize) {
    // one already canonical one
    test_kmer("AGGAAAGTCTTTTACGCTGGGGTAAGAGTGA",
              "AGGAAAGTCTTTTACGCTGGGGTAAGAGTGA", true);
    // two k-mers which need to be flipped
    test_kmer("TGGAAAGTCTTTTACGCTGGGGTAAGAGTGA",
              "TCACTCTTACCCCAGCGTAAAAGACTTTCCA", true);
    test_kmer("TTTTTTGTCTTTTACGCTGGGGTTTAAAAAA",
              "TTTTTTAAACCCCAGCGTAAAAGACAAAAAA", true);
    // special case, lexicographically smaller until center
    test_kmer("AAAAAAAAAAAAAAAATTTTTTTTTTTTTTT",
              "AAAAAAAAAAAAAAAATTTTTTTTTTTTTTT", true);

    // one kmer already canonical but containing invalid letters
    test_kmer("AGGAAAGTCTTTTACGCTGGGXXXAGAGTGA",
              "AGGAAAGTCTTTTACGCTGGG\0\0\0AGAGTGA", false);
    // one k-mer needing flipping containing invalid letters
    test_kmer("TGGAAAGTCTTTTACGCTGGGXXXAGAGTGA",
              "TCACTCT\0\0\0CCCAGCGTAAAAGACTTTCCA", false);
    // one kmer containing the invalid letter at the center
    test_kmer("AAAAAAAAAAAAAAAXTTTTTTTTTTTTTTT",
              "AAAAAAAAAAAAAAA\0TTTTTTTTTTTTTTT", false);
}

/******************************************************************************/
