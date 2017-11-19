#include <gtest/gtest.h>
#include <sample.hpp>
#include <kmer.hpp>
#include <isi/classic_index.hpp>
#include <iostream>
#include <experimental/filesystem>
#include <util.hpp>
#include <file/sample_header.hpp>
#include <file/frequency_header.hpp>
#include <file/util.hpp>
#include <xxhash.h>

namespace {
    std::experimental::filesystem::path in_dir_1("test/resources/classic_index/input_1/");
    std::experimental::filesystem::path in_dir_2("test/resources/classic_index/input_2/");
    std::experimental::filesystem::path in_dir_3("test/resources/classic_index/input_3/");
    std::experimental::filesystem::path out_dir("test/out/classic_index/");
    std::experimental::filesystem::path out_file_1(out_dir.string() + "[sample_1-sample_3].g_isi");
    std::experimental::filesystem::path out_file_2(out_dir.string() + "[sample_4-sample_4].g_isi");
    std::experimental::filesystem::path out_file_3(out_dir.string() + "[sample_1-sample_9].g_isi");
    std::experimental::filesystem::path out_file_4(out_dir.string() + "[sample_9-sample_9].g_isi");
    std::experimental::filesystem::path out_file_5(out_dir.string() + "[sample_1-sample_8].g_isi");
    std::experimental::filesystem::path out_file_6(out_dir.string() + "[[sample_1-sample_8]-[sample_9-sample_9]].g_isi");
    std::experimental::filesystem::path out_file_7(out_dir.string() + "[[[sample_1-sample_3]-[sample_4-sample_4]]-[[sample_9-sample_9]-[sample_9-sample_9]]].g_isi");
    std::experimental::filesystem::path sample_1(in_dir_1.string() + "sample_1.g_sam");
    std::experimental::filesystem::path sample_2(in_dir_1.string() + "sample_2.g_sam");
    std::experimental::filesystem::path sample_3(in_dir_1.string() + "sample_3.g_sam");
    std::experimental::filesystem::path sample_4(in_dir_2.string() + "sample_4.g_sam");
    std::experimental::filesystem::path sample_8(in_dir_3.string() + "sample_8.g_sam");
    std::experimental::filesystem::path sample_9(in_dir_3.string() + "sample_9.g_sam");
    size_t signature_size = 200000;
    size_t block_size = 1;
    size_t num_hashes = 7;

    class classic_index : public ::testing::Test {
    protected:
        virtual void SetUp() {
            std::error_code ec;
            std::experimental::filesystem::remove_all(out_dir, ec);
        }
    };

    bool is_set(const std::vector<uint8_t>& data, uint64_t block_size, size_t pos, size_t bit_in_block) {
        uint8_t b = data[block_size * pos + bit_in_block / 8];
        return (b & (1 << (bit_in_block % 8))) != 0;
    };

    bool contains(const std::vector<uint8_t>& data, const isi::file::classic_index_header& h, const isi::kmer<31>& kmer, size_t bit_in_block) {
        assert(bit_in_block < 8 * h.block_size());
        for (unsigned int k = 0; k < h.num_hashes(); k++) {
            size_t hash = XXH32(&kmer.data(), 8, k);
            if (!is_set(data, h.block_size(), hash % h.signature_size(), bit_in_block)) {
                return false;
            }
        }
        return true;
    }

    TEST_F(classic_index, contains) {
        isi::classic_index::create_from_samples(in_dir_1, out_dir, signature_size, block_size, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_1, isi, h);

        isi::sample<31> s;
        isi::file::deserialize(sample_1, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 0));
        }

        isi::file::deserialize(sample_2, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 1));
        }

        isi::file::deserialize(sample_3, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 2));
        }
    }

    TEST_F(classic_index, file_names) {
        isi::classic_index::create_from_samples(in_dir_1, out_dir, signature_size, block_size, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_1, isi, h);

        std::cout << h.file_names().size() << std::endl;
        for(auto fn: h.file_names()) {
            std::cout << fn << std::endl;
        }

        ASSERT_EQ(h.file_names()[0], "sample_1");
        ASSERT_EQ(h.file_names()[1], "sample_2");
        ASSERT_EQ(h.file_names()[2], "sample_3");
    }

    TEST_F(classic_index, false_positive) {
        isi::classic_index::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_2, isi, h);

        size_t num_tests = 100000;
        size_t num_positive = 0;
        for (size_t i = 0; i < num_tests; i++) {
            std::array<uint8_t, 8> a = {(uint8_t) (i >> 0), (uint8_t) (i >> 8), (uint8_t) (i >> 16), (uint8_t) (i >> 24),
                                     (uint8_t) (i >> 32), (uint8_t) (i >> 40), (uint8_t) (i >> 48), (uint8_t) (i >> 56)};
            isi::kmer<31> k(a);
            if (contains(isi, h, k, 0)) {
                num_positive++;
            }
        }
        ASSERT_EQ(num_positive, 945U);
    }

    TEST_F(classic_index, equal_ones_and_zeros) {
        isi::classic_index::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::deserialize(out_file_2, isi);

        size_t ones = 0;
        for (auto b: isi) {
            ASSERT_TRUE(b == 0 || b == 1);
            ones += b;
        }
        size_t zeros = isi.size() - ones;
        ASSERT_EQ(zeros, 97734U);
        ASSERT_EQ(ones, 102266U);
    }

    TEST_F(classic_index, others_zero) {
        isi::classic_index::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_2, isi, h);

        for (size_t i = 0; i < 100000; i++) {
            std::array<uint8_t, 8> a = {(uint8_t) (i >> 0), (uint8_t) (i >> 8), (uint8_t) (i >> 16), (uint8_t) (i >> 24),
                                     (uint8_t) (i >> 32), (uint8_t) (i >> 40), (uint8_t) (i >> 48), (uint8_t) (i >> 56)};
            isi::kmer<31> k(a);
            ASSERT_FALSE(contains(isi, h, k, i % 5 + 3));
        }
    }

    TEST_F(classic_index, contains_big_isi) {
        isi::classic_index::create_from_samples(in_dir_3, out_dir, signature_size, 2, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_3, isi, h);

        isi::sample<31> s;
        isi::file::deserialize(sample_9, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 8));
        }
    }

    TEST_F(classic_index, two_outputs) {
        isi::classic_index::create_from_samples(in_dir_3, out_dir, signature_size, block_size, num_hashes);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_4, isi, h);

        isi::sample<31> s;
        isi::file::deserialize(sample_9, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 0));
        }
    }

    TEST_F(classic_index, multi_level_1) {
        isi::classic_index::create_from_samples(in_dir_3, out_dir, signature_size, block_size, num_hashes);
        isi::classic_index::combine(out_dir, out_dir, signature_size, num_hashes, 2);

        std::vector<uint8_t> isi_1;
        isi::file::deserialize(out_file_5, isi_1);
        std::vector<uint8_t> isi_2;
        isi::file::deserialize(out_file_4, isi_2);
        std::vector<uint8_t> isi_3;
        isi::file::deserialize(out_file_6, isi_3);

        for (size_t i = 0; i < signature_size; i++) {
            ASSERT_EQ(*(isi_1.data() + i), *(isi_3.data() + 2 * i));
            ASSERT_EQ(*(isi_2.data() + i), *(isi_3.data() + 2 * i + 1));
        }
    }

    TEST_F(classic_index, multi_level_2) {
        isi::classic_index::create_from_samples(in_dir_1, out_dir, signature_size, block_size, num_hashes);
        isi::classic_index::create_from_samples(in_dir_2, out_dir, signature_size, block_size, num_hashes);
        isi::classic_index::create_from_samples(in_dir_3, out_dir, signature_size, block_size, num_hashes);
        isi::classic_index::combine(out_dir, out_dir, signature_size, num_hashes, 3);
        isi::classic_index::combine(out_dir, out_dir, signature_size, num_hashes, 2);

        std::vector<uint8_t> isi;
        isi::file::classic_index_header h;
        isi::file::deserialize(out_file_7, isi, h);

        isi::sample<31> s;
        isi::file::deserialize(sample_3, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 2));
        }

        isi::file::deserialize(sample_4, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 16));
        }

        isi::file::deserialize(sample_8, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 15));
        }

        isi::file::deserialize(sample_9, s);
        for (auto kmer: s.data()) {
            ASSERT_TRUE(contains(isi, h, kmer, 24));
        }
    }
}
