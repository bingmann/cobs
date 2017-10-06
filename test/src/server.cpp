#include <gtest/gtest.h>
#include <server/server_mmap.hpp>
#include <iostream>


namespace {
    using namespace genome;

    std::string in_dir = "test/resources/server/input/";
    std::string sample = in_dir + "sample.g_blo";
    std::string sample_result = "test/resources/server/result/sample_sorted.txt";
    std::string file_name = "ERR102554";

    TEST(server, sample) {
        std::vector<std::pair<double, std::string>> result_mmap;
        std::vector<std::pair<double, std::string>> result_ifs;
        server_mmap s_mmap(sample);
        server_mmap s_ifs(sample);

        std::ifstream ifs(sample_result);
        std::string line;

        while (std::getline(ifs, line)) {
            s_mmap.search_bloom_filter<31>(line, result_mmap);
            s_ifs.search_bloom_filter<31>(line, result_ifs);
            double res = 0;
            ASSERT_EQ(result_mmap.size(), result_ifs.size());
            for (size_t i = 0; i < result_mmap.size(); i++) {
                ASSERT_EQ(result_mmap[i], result_ifs[i]);
                if (result_mmap[i].second == file_name) {
                    res = result_mmap[i].first;
                }
            }
            ASSERT_TRUE(res);
        }
    }
}
