#include <gtest/gtest.h>
#include <sample.hpp>
#include <kmer.hpp>
#include <bloom_filter.hpp>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <helpers.hpp>

namespace {
    using namespace genome;

    std::string in_dir_1 = "test/resources/bloom_filter/input_1/";
    std::string in_dir_2 = "test/resources/bloom_filter/input_2/";
    std::string in_dir_3 = "test/resources/bloom_filter/input_3/";
    std::string out_dir = "test/out/bloom_filter/";
    std::string out_file_1 = out_dir + "[sample_1-sample_3].bl";
    std::string out_file_2 = out_dir + "[sample_4-sample_4].bl";
    std::string out_file_3 = out_dir + "[sample_1-sample_9].bl";
    std::string sample_1 = in_dir_1 + "sample_1.b";
    std::string sample_2 = in_dir_1 + "sample_2.b";
    std::string sample_3 = in_dir_1 + "sample_3.b";
    std::string sample_4 = in_dir_2 + "sample_4.b";
    std::string sample_9 = in_dir_3 + "sample_9.b";


    TEST(bloom_filter, contains) {
        boost::filesystem::remove_all(out_dir);
        genome::bloom_filter::process_all_in_directory<200000, 1, 7>(in_dir_1, out_dir);

        std::vector<byte> signature;
        read_file(out_file_1, signature);

        genome::sample<31> s;

        read_file(sample_1, s.data());
        for (auto kmer: s.data()) {
            bool contains = genome::bloom_filter::contains<200000, 1, 7>(signature, kmer, 0);
            ASSERT_TRUE(contains);
        }

        read_file(sample_2, s.data());
        for (auto kmer: s.data()) {
            bool contains = genome::bloom_filter::contains<200000, 1, 7>(signature, kmer, 1);
            ASSERT_TRUE(contains);
        }

        read_file(sample_3, s.data());
        for (auto kmer: s.data()) {
            bool contains = genome::bloom_filter::contains<200000, 1, 7>(signature, kmer, 2);
            ASSERT_TRUE(contains);
        }
    }

    TEST(bloom_filter, contains_big_block) {
        boost::filesystem::remove_all(out_dir);
        genome::bloom_filter::process_all_in_directory<200000, 3, 7>(in_dir_3, out_dir);

        std::vector<byte> signature;
        read_file(out_file_3, signature);

        genome::sample<31> s;

        read_file(sample_9, s.data());
        for (auto kmer: s.data()) {
            bool contains = genome::bloom_filter::contains<200000, 3, 7>(signature, kmer, 8);
            ASSERT_TRUE(contains);
        }
    }

    TEST(bloom_filter, false_positive) {
        boost::filesystem::remove_all(out_dir);
        genome::bloom_filter::process_all_in_directory<200000, 1, 7>(in_dir_1, out_dir);

        std::vector<byte> signature;
        read_file(out_file_1, signature);

        size_t num_tests = 100000;
        size_t num_positive = 0;
        for (size_t i = 0; i < num_tests; i++) {
            std::array<byte, 8> a = {(byte) (i >> 0), (byte) (i >> 8), (byte) (i >> 16), (byte) (i >> 24),
                                     (byte) (i >> 32), (byte) (i >> 40), (byte) (i >> 48), (byte) (i >> 56)};
            kmer<31> k(a);
            if (genome::bloom_filter::contains<200000, 1, 7>(signature, k, 0)) {
                num_positive++;
            }
        }
        ASSERT_EQ(num_positive, 945);
    }

    TEST(bloom_filter, equal_ones_and_zeros) {
        boost::filesystem::remove_all(out_dir);
        genome::bloom_filter::process_all_in_directory<200000, 1, 7>(in_dir_2, out_dir);

        std::vector<byte> signature;
        read_file(out_file_2, signature);
        size_t ones = 0;
        for (auto b: signature) {
            ASSERT_TRUE(b == 0 || b == 1);
            ones += b;
        }
        size_t zeros = signature.size() - ones;
        ASSERT_EQ(zeros, 97734);
        ASSERT_EQ(ones, 102266);
    }

    TEST(bloom_filter, others_zero) {
        boost::filesystem::remove_all(out_dir);
        genome::bloom_filter::process_all_in_directory<200000, 1, 7>(in_dir_2, out_dir);

        std::vector<byte> signature;
        read_file(out_file_2, signature);

        size_t num_positive = 0;
        for (size_t i = 0; i < 100000; i++) {
            std::array<byte, 8> a = {(byte) (i >> 0), (byte) (i >> 8), (byte) (i >> 16), (byte) (i >> 24),
                                     (byte) (i >> 32), (byte) (i >> 40), (byte) (i >> 48), (byte) (i >> 56)};
            kmer<31> k(a);
            bool contains = genome::bloom_filter::contains<200000, 1, 7>(signature, k, i % 5 + 3);
            ASSERT_FALSE(contains);
        }
    }
}
