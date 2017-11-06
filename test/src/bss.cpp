#include <gtest/gtest.h>
#include <sample.hpp>
#include <kmer.hpp>
#include <bit_sliced_signatures/bss.hpp>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <util.hpp>
#include <file/sample_header.hpp>
#include <file/frequency_header.hpp>
#include <file/util.hpp>

namespace {
    std::string in_dir_1 = "test/resources/bss/input_1/";
    std::string in_dir_2 = "test/resources/bss/input_2/";
    std::string in_dir_3 = "test/resources/bss/input_3/";
    std::string out_dir = "test/out/bss/";
    std::string out_file_1 = out_dir + "[sample_1-sample_3].g_bss";
    std::string out_file_2 = out_dir + "[sample_4-sample_4].g_bss";
    std::string out_file_3 = out_dir + "[sample_1-sample_9].g_bss";
    std::string out_file_4 = out_dir + "[sample_9-sample_9].g_bss";
    std::string out_file_5 = out_dir + "[sample_1-sample_8].g_bss";
    std::string out_file_6 = out_dir + "[[sample_1-sample_8]-[sample_9-sample_9]].g_bss";
    std::string out_file_7 = out_dir + "[[[sample_1-sample_3]-[sample_4-sample_4]]-[[sample_9-sample_9]-[sample_9-sample_9]]].g_bss";
    std::string sample_1 = in_dir_1 + "sample_1.g_sam";
    std::string sample_2 = in_dir_1 + "sample_2.g_sam";
    std::string sample_3 = in_dir_1 + "sample_3.g_sam";
    std::string sample_4 = in_dir_2 + "sample_4.g_sam";
    std::string sample_8 = in_dir_3 + "sample_8.g_sam";
    std::string sample_9 = in_dir_3 + "sample_9.g_sam";
    size_t signature_size = 200000;
    size_t block_size = 1;
    size_t num_hashes = 7;


    TEST(bss, contains) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_1, out_dir, signature_size, block_size, num_hashes);

        genome::bss bss;
        genome::file::deserialize(out_file_1, bss);

        genome::sample<31> s;
        genome::file::deserialize(sample_1, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 0));
        }

        genome::file::deserialize(sample_2, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 1));
        }

        genome::file::deserialize(sample_3, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 2));
        }
    }

    TEST(bss, file_names) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_1, out_dir, signature_size, block_size, num_hashes);

        genome::bss bss;
        genome::file::bss_header bssh;
        genome::file::deserialize(out_file_1, bss, bssh);

        ASSERT_EQ(bssh.file_names()[0], "sample_1");
        ASSERT_EQ(bssh.file_names()[1], "sample_2");
        ASSERT_EQ(bssh.file_names()[2], "sample_3");
    }


    TEST(bss, false_positive) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);

        genome::bss bss;
        genome::file::deserialize(out_file_2, bss);

        size_t num_tests = 100000;
        size_t num_positive = 0;
        for (size_t i = 0; i < num_tests; i++) {
            std::array<genome::byte, 8> a = {(genome::byte) (i >> 0), (genome::byte) (i >> 8), (genome::byte) (i >> 16), (genome::byte) (i >> 24),
                                     (genome::byte) (i >> 32), (genome::byte) (i >> 40), (genome::byte) (i >> 48), (genome::byte) (i >> 56)};
            genome::kmer<31> k(a);
            if (bss.contains(k, 0)) {
                num_positive++;
            }
        }
        ASSERT_EQ(num_positive, 945U);
    }

    TEST(bss, equal_ones_and_zeros) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);

        genome::bss bss;
        genome::file::deserialize(out_file_2, bss);

        size_t ones = 0;
        for (auto b: bss.data()) {
            ASSERT_TRUE(b == 0 || b == 1);
            ones += b;
        }
        size_t zeros = bss.data().size() - ones;
        ASSERT_EQ(zeros, 97734U);
        ASSERT_EQ(ones, 102266U);
    }

    TEST(bss, others_zero) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);

        genome::bss bss;
        genome::file::deserialize(out_file_2, bss);

        for (size_t i = 0; i < 100000; i++) {
            std::array<genome::byte, 8> a = {(genome::byte) (i >> 0), (genome::byte) (i >> 8), (genome::byte) (i >> 16), (genome::byte) (i >> 24),
                                     (genome::byte) (i >> 32), (genome::byte) (i >> 40), (genome::byte) (i >> 48), (genome::byte) (i >> 56)};
            genome::kmer<31> k(a);
            ASSERT_FALSE(bss.contains(k, i % 5 + 3));
        }
    }

    TEST(bss, contains_big_bss) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_3, out_dir, signature_size, 2, num_hashes);

        genome::bss bss;
        genome::file::deserialize(out_file_3, bss);

        genome::sample<31> s;
        genome::file::deserialize(sample_9, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 8));
        }
    }

    TEST(bss, two_outputs) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_3, out_dir, signature_size, block_size, num_hashes);

        genome::bss bss;
        genome::file::deserialize(out_file_4, bss);

        genome::sample<31> s;
        genome::file::deserialize(sample_9, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 0));
        }
    }

    TEST(bss, multi_level_1) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_3, out_dir, signature_size, block_size, num_hashes);
        genome::bss::combine_bss(out_dir, out_dir, signature_size, num_hashes, 2);

        genome::bss bss_1;
        genome::file::deserialize(out_file_5, bss_1);
        genome::bss bss_2;
        genome::file::deserialize(out_file_4, bss_2);
        genome::bss bss_3;
        genome::file::deserialize(out_file_6, bss_3);

        for (size_t i = 0; i < signature_size; i++) {
            ASSERT_EQ(*(bss_1.data().data() + i), *(bss_3.data().data() + 2 * i));
            ASSERT_EQ(*(bss_2.data().data() + i), *(bss_3.data().data() + 2 * i + 1));
        }
    }

    TEST(bss, multi_level_2) {
        boost::filesystem::remove_all(out_dir);
        genome::bss::create_from_samples(in_dir_1, out_dir, signature_size, block_size, num_hashes);
        genome::bss::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);
        genome::bss::create_from_samples(in_dir_3, out_dir, signature_size, block_size, num_hashes);
        genome::bss::combine_bss(out_dir, out_dir, signature_size, num_hashes, 3);
        genome::bss::combine_bss(out_dir, out_dir, signature_size, num_hashes, 2);

        genome::bss bss;
        genome::file::deserialize(out_file_7, bss);

        genome::sample<31> s;
        genome::file::deserialize(sample_3, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 2));
        }

        genome::file::deserialize(sample_4, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 16));
        }

        genome::file::deserialize(sample_8, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 15));
        }

        genome::file::deserialize(sample_9, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(bss.contains(kmer, 24));
        }
    }
}
