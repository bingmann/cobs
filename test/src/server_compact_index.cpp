#include <gtest/gtest.h>
#include "test_util.hpp"

namespace {
    std::experimental::filesystem::path in_dir("test/out/server_compact_index/input_1/");
    std::experimental::filesystem::path compact_index_path(in_dir.string() + "filter.g_cisi");
    std::experimental::filesystem::path tmp_dir("test/out/server_compact_index/tmp");
    std::string query = isi::random_sequence(50000, 1);

    class server_compact_index : public ::testing::Test {
    protected:
        virtual void SetUp() {
            std::error_code ec;
            std::experimental::filesystem::remove_all(in_dir, ec);
            std::experimental::filesystem::remove_all(tmp_dir, ec);
            std::experimental::filesystem::create_directories(in_dir);
            std::experimental::filesystem::create_directories(tmp_dir);
        }
    };

    TEST_F(server_compact_index, all_included) {

        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::compact_index::mmap s_mmap(compact_index_path);

        std::vector<std::pair<uint16_t, std::string>> result;
        s_mmap.search(query, 31, result);
        std::vector<std::string> split;
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            int index = std::stoi(r.second.substr(r.second.size() - 2));
            ASSERT_GE(r.first, samples[index].data().size());
        }
    }

    TEST_F(server_compact_index, one_included) {
        auto samples = generate_samples_one(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::compact_index::mmap s_mmap(compact_index_path);

        std::vector<std::pair<uint16_t, std::string>> result;
        s_mmap.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            ASSERT_EQ(r.first, 1);
        }
    }

    TEST_F(server_compact_index, false_positive) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::compact_index::mmap s_mmap(compact_index_path);

        size_t num_tests = 10000;
        std::map<std::string, uint64_t> num_positive;
        std::vector<std::pair<uint16_t, std::string>> result;
        for (size_t i = 0; i < num_tests; i++) {
            std::string query_2 = isi::random_sequence(31, i);
            s_mmap.search(query_2, 31, result);

            for (auto& r: result) {
                num_positive[r.second] += r.first;
                ASSERT_TRUE(r.first == 0 || r.first == 1);
            }
        }

        for (auto& np: num_positive) {
            ASSERT_LE(np.second, 1030U);
        }
    }
}
