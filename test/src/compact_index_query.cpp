#include <gtest/gtest.h>
#include "test_util.hpp"
#ifdef __linux__
#include <cobs/query/compact_index/aio.hpp>
#endif

namespace {
    namespace fs = cobs::fs;
    fs::path in_dir("test/out/compact_index_query/input");
    fs::path index_path(in_dir.string() + "/index.com_idx.isi");
    fs::path tmp_dir("test/out/compact_index_query/tmp");
    std::string query = cobs::random_sequence(21000, 1);

    class compact_index_query : public ::testing::Test {
    protected:
        virtual void SetUp() {
            cobs::error_code ec;
            fs::remove_all(in_dir, ec);
            if (ec) {
                std::cout << ec.message() << std::endl;
            }
            fs::remove_all(tmp_dir, ec);
            if (ec) {
                std::cout << ec.message() << std::endl;
            }
            fs::create_directories(in_dir);
            fs::create_directories(tmp_dir);
        }
    };

    TEST_F(compact_index_query, all_included_mmap) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir.string());
        cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
        cobs::compact_index::create_from_folders(in_dir, 8, 3, 0.1, 2);
        cobs::query::compact_index::mmap s_mmap(index_path);

        std::vector<std::pair<uint16_t, std::string>> result;
        s_mmap.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            int index = std::stoi(r.second.substr(r.second.size() - 2));
            ASSERT_GE(r.first, samples[index].data().size());
        }
    }

    TEST_F(compact_index_query, one_included_mmap) {
        auto samples = generate_samples_one(query);
        generate_test_case(samples, tmp_dir.string());
        cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
        cobs::compact_index::create_from_folders(in_dir, 8, 3, 0.1, 2);
        cobs::query::compact_index::mmap s_mmap(index_path);

        std::vector<std::pair<uint16_t, std::string>> result;
        s_mmap.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            ASSERT_EQ(r.first, 1);
        }
    }

    TEST_F(compact_index_query, false_positive_mmap) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir.string());
        cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
        cobs::compact_index::create_from_folders(in_dir, 8, 3, 0.1, 2);
        cobs::query::compact_index::mmap s_mmap(index_path);

        size_t num_tests = 10000;
        std::map<std::string, uint64_t> num_positive;
        std::vector<std::pair<uint16_t, std::string>> result;
        for (size_t i = 0; i < num_tests; i++) {
            std::string query_2 = cobs::random_sequence(31, i);
            s_mmap.search(query_2, 31, result);

            for (auto& r: result) {
                num_positive[r.second] += r.first;
                ASSERT_TRUE(r.first == 0 || r.first == 1);
            }
        }

        for (auto& np: num_positive) {
            ASSERT_LE(np.second, 1070U);
        }
    }

#ifdef __linux__
    TEST_F(compact_index_query, all_included_aio) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir.string());
        cobs::compact_index::create_folders(tmp_dir, in_dir, 4096);
        cobs::compact_index::create_from_folders(in_dir, 8, 3, 0.1, 4096);
        cobs::query::compact_index::aio s_aio(index_path);

        std::vector<std::pair<uint16_t, std::string>> result;
        s_aio.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            int index = std::stoi(r.second.substr(r.second.size() - 2));
            ASSERT_GE(r.first, samples[index].data().size());
        }
    }

    TEST_F(compact_index_query, one_included_aio) {
        auto samples = generate_samples_one(query);
        generate_test_case(samples, tmp_dir.string());
        cobs::compact_index::create_folders(tmp_dir, in_dir, 4096);
        cobs::compact_index::create_from_folders(in_dir, 8, 3, 0.1, 4096);
        cobs::query::compact_index::mmap s_aio(index_path);

        std::vector<std::pair<uint16_t, std::string>> result;
        s_aio.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            ASSERT_EQ(r.first, 1);
        }
    }

    TEST_F(compact_index_query, false_positive_aio) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir.string());
        cobs::compact_index::create_folders(tmp_dir, in_dir, 4096);
        cobs::compact_index::create_from_folders(in_dir, 8, 3, 0.1, 4096);
        cobs::query::compact_index::mmap s_aio(index_path);

        size_t num_tests = 10000;
        std::map<std::string, uint64_t> num_positive;
        std::vector<std::pair<uint16_t, std::string>> result;
        for (size_t i = 0; i < num_tests; i++) {
            std::string query_2 = cobs::random_sequence(31, i);
            s_aio.search(query_2, 31, result);

            for (auto& r: result) {
                num_positive[r.second] += r.first;
                ASSERT_TRUE(r.first == 0 || r.first == 1);
            }
        }

        for (auto& np: num_positive) {
            ASSERT_LE(np.second, 1070U);
        }
    }
#endif
}
