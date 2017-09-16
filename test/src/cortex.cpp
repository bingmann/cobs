#include <gtest/gtest.h>
#include <cortex.hpp>
#include <iostream>
#include <boost/filesystem/operations.hpp>

namespace {
    std::string in_path = "test/resources/sample.ctx";
    std::string out_path = "test/out/sample.b";
    std::string in_dir = "test/resources/";
    std::string out_dir = "test/out/";
    std::string sample_path = "test/resources/sample_sorted.txt";

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
        cortex::deserialize(sample1, out_path);
        cortex::deserialize(sample2, out_path);
        assert_equals_sample(sample1);
        assert_equals_sample(sample2);
    }

    TEST(cortex, process_all_in_directory) {
        boost::filesystem::remove_all(out_dir);
        cortex::process_all_in_directory<31>(in_dir, out_dir);
        sample<31> sample;
        cortex::deserialize(sample, out_dir + "sample_rec.b");
        assert_equals_sample(sample);
    }
}
