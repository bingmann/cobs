/*******************************************************************************
 * test/util.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/kmer.hpp>
#include <cobs/util/misc.hpp>
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

void test_kmer(const char* kmer_data, bool flipped) {
    cobs::KMer<31> kmer1(kmer_data);
    die_unequal(kmer1.string(), kmer_data);

    std::string kmer_at_test;
    char letter[4] = { 'A', 'C', 'G', 'T' };
    for (size_t i = 0; i < 31; ++i) {
        kmer_at_test += letter[kmer1.at(i)];
    }
    die_unequal(kmer1.string(), kmer_at_test);

    // check canonicalization
    char kmer_buffer[31];
    const char* kmer_canon = cobs::canonicalize_kmer(
        kmer_data, kmer_buffer, 31);

    kmer1.canonicalize();
    die_unequal(kmer_canon, kmer1.string());

    if (!flipped)
        die_unequal(kmer_data, kmer1.string());
    else
        die_equal(kmer_data, kmer1.string());
}

TEST(util, kmer_canonicalize) {
    test_kmer("AGGAAAGTCTTTTACGCTGGGGTAAGAGTGA", false);
    test_kmer("TGGAAAGTCTTTTACGCTGGGGTAAGAGTGA", true);
    test_kmer("TTTTTTGTCTTTTACGCTGGGGTTTAAAAAA", true);
}

/******************************************************************************/
