#include <gtest/gtest.h>
#include <server/classic_index/mmap.hpp>
#include <iostream>


namespace {
    std::string in_dir = "test/resources/server_classic_index/input/";
    std::string sample_1 = in_dir + "sample_1.g_isi";
    std::string sample_2 = in_dir + "sample_2.g_isi";
    std::string sample_result = "test/resources/server_classic_index/result/sample_sorted.txt";
    std::string file_name = "ERR102554";

    TEST(server_classic_index, block_size_4) {
        std::vector<std::pair<uint16_t, std::string>> result_map;
        std::vector<std::pair<uint16_t, std::string>> result_ifs;
        isi::server::classic_index::mmap s_mmap(sample_1);
        isi::server::classic_index::mmap s_ifs(sample_1);

        std::ifstream ifs(sample_result);
        std::string line;

        while (std::getline(ifs, line)) {
            s_mmap.search(line, 31, result_map);
            s_ifs.search(line, 31, result_ifs);
            double res = 0;
            ASSERT_EQ(result_map.size(), result_ifs.size());
            for (size_t i = 0; i < result_map.size(); i++) {
                ASSERT_EQ(result_map[i], result_ifs[i]);
                if (result_map[i].second == file_name) {
                    res = result_map[i].first;
                }
            }
            ASSERT_TRUE(res);
        }
    }

    TEST(server_classic_index, block_size_15) {
        std::vector<std::pair<uint16_t, std::string>> result_map;
        std::vector<std::pair<uint16_t, std::string>> result_ifs;
        isi::server::classic_index::mmap s_mmap(sample_2);
        isi::server::classic_index::mmap s_ifs(sample_2);

        std::ifstream ifs(sample_result);
        std::string line;

        while (std::getline(ifs, line)) {
            s_mmap.search(line, 31, result_map);
            s_ifs.search(line, 31, result_ifs);
            double res = 0;
            ASSERT_EQ(result_map.size(), result_ifs.size());
            for (size_t i = 0; i < result_map.size(); i++) {
                ASSERT_EQ(result_map[i], result_ifs[i]);
                if (result_map[i].second == file_name) {
                    res = result_map[i].first;
                }
            }
            ASSERT_TRUE(res);
        }
    }
}
