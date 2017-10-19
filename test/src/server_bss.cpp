#include <gtest/gtest.h>
#include <server/bss/mmap.hpp>
#include <iostream>


namespace {
    std::string in_dir = "test/resources/server/input/";
    std::string sample_1 = in_dir + "sample_1.g_bss";
    std::string sample_2 = in_dir + "sample_2.g_bss";
    std::string sample_result = "test/resources/server/result/sample_sorted.txt";
    std::string file_name = "ERR102554";

//    TEST(server, block_size_4) {
//        std::vector<std::pair<uint16_t, std::string>> result_map;
//        std::vector<std::pair<uint16_t, std::string>> result_ifs;
//        genome::server::bss::mmap s_mmap(sample_1);
//        genome::server::bss::mmap s_ifs(sample_1);
//
//        std::ifstream ifs(sample_result);
//        std::string line;
//
//        while (std::getline(ifs, line)) {
//            s_mmap.search_bss(line, 31, result_map);
//            s_ifs.search_bss(line, 31, result_ifs);
//            double res = 0;
//            ASSERT_EQ(result_map.size(), result_ifs.size());
//            for (size_t i = 0; i < result_map.size(); i++) {
//                ASSERT_EQ(result_map[i], result_ifs[i]);
//                if (result_map[i].second == file_name) {
//                    res = result_map[i].first;
//                }
//            }
//            ASSERT_TRUE(res);
//        }
//    }

    TEST(server, block_size_15) {
        std::vector<std::pair<uint16_t, std::string>> result_map;
        std::vector<std::pair<uint16_t, std::string>> result_ifs;
        genome::server::bss::mmap s_mmap(sample_2);
        genome::server::bss::mmap s_ifs(sample_2);

        std::ifstream ifs(sample_result);
        std::string line;

        while (std::getline(ifs, line)) {
            s_mmap.search_bss(line, 31, result_map);
            s_ifs.search_bss(line, 31, result_ifs);
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
