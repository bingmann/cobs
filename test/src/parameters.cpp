#include <gtest/gtest.h>
#include <isi/util.hpp>
#include <isi/construction/classic_index_construction.hpp>
#include <xxhash.h>
#include <isi/util/parameters.hpp>

namespace {
    size_t get_num_positives(uint64_t num_elements, uint64_t num_hashes, double false_positive_probability, size_t num_tests) {
        uint64_t signature_size = isi::calc_signature_size(num_elements, num_hashes, false_positive_probability);

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

    size_t get_num_positives_hash(uint64_t num_hashes, double false_positive_probability, size_t num_tests) {
        std::string query = isi::random_sequence(10000, 1);
        uint64_t num_elements = query.size() - 30;
        uint64_t signature_size = isi::calc_signature_size(num_elements, num_hashes, false_positive_probability);

        std::vector<bool> signature(signature_size);
        isi::kmer<31> k;
        for (size_t i = 0; i < num_elements; i++) {
            isi::classic_index::create_hashes(query.data() + i, 31, signature_size, num_hashes, [&](size_t index) {
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
}
