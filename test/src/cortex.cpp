#include <gtest/gtest.h>
#include <isi/cortex.hpp>
#include <iostream>
#include <experimental/filesystem>
#include <isi/util.hpp>
#include <isi/util/file.hpp>


namespace {
    std::string in_dir = "test/resources/cortex/input/";
    std::string out_dir = "test/out/cortex/";
    std::string in_path = in_dir + "sample.ctx";
    std::string out_path = out_dir + "sample.sam.isi";
    std::string out_path_rec = out_dir + "sample_rec.sam.isi";
    std::string sample_path = "test/resources/cortex/result/sample_sorted.txt";

    template<unsigned int N>
    void assert_equals_sample(isi::sample<N> sample) {
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
        isi::sample<31> sample;
        isi::cortex::process_file(in_path, out_path, sample);
        assert_equals_sample(sample);
    }

    TEST(cortex, file_serialization) {
        std::experimental::filesystem::remove_all(out_dir);
        isi::sample<31> sample1;
        isi::sample<31> sample2;
        isi::cortex::process_file(in_path, out_path, sample1);
        isi::file::deserialize(out_path, sample2);
        assert_equals_sample(sample1);
        assert_equals_sample(sample2);
    }

    TEST(cortex, process_all_in_directory) {
        std::experimental::filesystem::remove_all(out_dir);
        isi::cortex::process_all_in_directory<31>(in_dir, out_dir);
        isi::sample<31> sample;
        isi::file::deserialize(out_path_rec, sample);
        assert_equals_sample(sample);
    }
}
