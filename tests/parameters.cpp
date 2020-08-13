/*******************************************************************************
 * tests/parameters.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/kmer.hpp>
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/misc.hpp>
#include <cobs/util/process_file_batches.hpp>
#include <cobs/util/query.hpp>
#include <gtest/gtest.h>

#include <unordered_map>

static std::string query = cobs::random_sequence(10000, 1);

static std::unordered_map<char, char> basepairs = {
    { 'A', 'T' }, { 'C', 'G' }, { 'G', 'C' }, { 'T', 'A' }
};

size_t get_num_positives(uint64_t num_elements, uint64_t num_hashes, double false_positive_rate, size_t num_tests) {
    uint64_t signature_size = cobs::calc_signature_size(num_elements, num_hashes, false_positive_rate);

    std::vector<bool> signature(signature_size);
    std::srand(1);
    for (size_t i = 0; i < num_hashes * num_elements; i++) {
        signature[std::rand() % signature.size()] = true;
    }

    size_t num_positives = 0;
    for (size_t i = 0; i < num_tests; i++) {
        size_t num_hits = 0;
        for (size_t j = 0; j < num_hashes; j++) {
            num_hits += signature[std::rand() % signature.size()] ? 1 : 0;
        }
        num_positives += num_hits == num_hashes ? 1 : 0;
    }
    return num_positives;
}

size_t get_num_positives_hash(uint64_t num_hashes, double false_positive_rate, size_t num_tests) {
    std::string query = cobs::random_sequence(10000, 1);
    uint64_t num_elements = query.size() - 30;
    uint64_t signature_size = cobs::calc_signature_size(num_elements, num_hashes, false_positive_rate);

    std::vector<bool> signature(signature_size);
    cobs::KMer<31> k;
    for (size_t i = 0; i < num_elements; i++) {
        cobs::process_hashes(
            query.data() + i, 31, signature_size, num_hashes,
            [&](size_t index) {
                signature[index] = true;
            });
    }

    std::srand(1);
    size_t num_positives = 0;
    for (size_t i = 0; i < num_tests; i++) {
        size_t num_hits = 0;
        for (size_t j = 0; j < num_hashes; j++) {
            num_hits += signature[std::rand() % signature.size()] ? 1 : 0;
        }
        num_positives += num_hits == num_hashes ? 1 : 0;
    }
    return num_positives;
}

void assert_between(size_t num, size_t min, size_t max) {
    ASSERT_GE(num, min);
    ASSERT_LE(num, max);
}

TEST(parameters, false_positive) {
    size_t num_positives = get_num_positives(100000, 1, 0.3, 100000);
    assert_between(num_positives, 29000, 31000);
    num_positives = get_num_positives(100000, 2, 0.3, 100000);
    assert_between(num_positives, 29000, 31000);
    num_positives = get_num_positives(100000, 3, 0.3, 100000);
    assert_between(num_positives, 29000, 31000);
    num_positives = get_num_positives(100000, 1, 0.1, 100000);
    assert_between(num_positives, 9800, 10200);
    num_positives = get_num_positives(100000, 2, 0.1, 100000);
    assert_between(num_positives, 9800, 10200);
    num_positives = get_num_positives(100000, 3, 0.1, 100000);
    assert_between(num_positives, 9800, 10200);
}

TEST(parameters, false_positive_hash) {
    size_t num_positives = get_num_positives_hash(1, 0.3, 100000);
    assert_between(num_positives, 29000, 31000);
    num_positives = get_num_positives_hash(2, 0.3, 100000);
    assert_between(num_positives, 29000, 31000);
    num_positives = get_num_positives_hash(3, 0.3, 100000);
    assert_between(num_positives, 29000, 31000);
    num_positives = get_num_positives_hash(1, 0.1, 100000);
    assert_between(num_positives, 9800, 10200);
    num_positives = get_num_positives_hash(2, 0.1, 100000);
    assert_between(num_positives, 9800, 10200);
    num_positives = get_num_positives_hash(3, 0.1, 100000);
    assert_between(num_positives, 9800, 10200);
}

TEST(parameters, canonical) {
    char kmer_buffer[31];
    for (size_t i = 0; i < query.size() - 31; i++) {
        char* kmer_8 = query.data() + i;
        bool good = cobs::canonicalize_kmer(kmer_8, kmer_buffer, 31);
        die_unless(good);

        std::string kmer_result(kmer_buffer, 31);
        std::string kmer_original(kmer_8, 31);
        std::string kmer_complement(31, 'X');
        for (size_t j = 0; j < 31; j++) {
            kmer_complement[j] = basepairs[kmer_original[30 - j]];
        }
        ASSERT_EQ(kmer_result, std::min(kmer_original, kmer_complement));
    }
}

/******************************************************************************/
