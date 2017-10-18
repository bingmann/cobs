#include <gtest/gtest.h>
#include <msbf.hpp>
#include <iostream>

namespace {
    using namespace genome;

    std::string in_dir_1 = "test/resources/bloom_filter/input_1/";
    std::string in_dir_2 = "test/resources/bloom_filter/input_2/";
    std::string in_dir_3 = "test/resources/bloom_filter/input_3/";
    std::string out_dir = "test/out/bloom_filter/";
    std::string out_file_1 = out_dir + "[sample_1-sample_3].g_blo";
    std::string out_file_2 = out_dir + "[sample_4-sample_4].g_blo";
    std::string out_file_3 = out_dir + "[sample_1-sample_9].g_blo";
    std::string out_file_4 = out_dir + "[sample_9-sample_9].g_blo";
    std::string out_file_5 = out_dir + "[sample_1-sample_8].g_blo";
    std::string out_file_6 = out_dir + "[[sample_1-sample_8]-[sample_9-sample_9]].g_blo";
    std::string out_file_7 =
            out_dir + "[[[sample_1-sample_3]-[sample_4-sample_4]]-[[sample_9-sample_9]-[sample_9-sample_9]]].g_blo";
    std::string sample_1 = in_dir_1 + "sample_1.g_sam";
    std::string sample_2 = in_dir_1 + "sample_2.g_sam";
    std::string sample_3 = in_dir_1 + "sample_3.g_sam";
    std::string sample_4 = in_dir_2 + "sample_4.g_sam";
    std::string sample_8 = in_dir_3 + "sample_8.g_sam";
    std::string sample_9 = in_dir_3 + "sample_9.g_sam";
    size_t bloom_filter_size = 200000;
    size_t block_size = 1;
    size_t num_hashes = 7;
}
