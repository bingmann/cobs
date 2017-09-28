#include <gtest/gtest.h>
#include <cortex.hpp>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <helpers.hpp>
#include <file/util.hpp>


namespace {
    using namespace genome;

    std::string in_dir = "test/resources/cortex/input/";
    std::string out_dir = "test/out/cortex/";
    std::string in_path = in_dir + "sample.ctx";
    std::string out_path = out_dir + "sample.g_sam";
    std::string out_path_rec = out_dir + "sample_rec.g_sam";
    std::string sample_path = "test/resources/cortex/result/sample_sorted.txt";

    template<unsigned int N>
    void assert_equals_sample(sample<N> sample) {
        std::ifstream ifs(sample_path);
        std::string line;
        size_t i = 0;
        while (std::getline(ifs, line)) {
            ASSERT_EQ(line, sample.data()[i].string());
            i++;
        }
        ASSERT_EQ(sample.data().size(), i);
    }

    TEST(cortex, process_file) {
        sample<31> sample;
        cortex::process_file(in_path, out_path, sample);
        assert_equals_sample(sample);
    }

    TEST(cortex, file_serialization) {
        boost::filesystem::remove_all(out_dir);
        sample<31> sample1;
        sample<31> sample2;
        cortex::process_file(in_path, out_path, sample1);
        genome::file::deserialize(out_path, sample2);
        assert_equals_sample(sample1);
        assert_equals_sample(sample2);
    }

    TEST(cortex, process_all_in_directory) {
        boost::filesystem::remove_all(out_dir);
        cortex::process_all_in_directory<31>(in_dir, out_dir);
        sample<31> sample;
        genome::file::deserialize(out_path_rec, sample);
        assert_equals_sample(sample);
    }
}
