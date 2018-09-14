#include <gtest/gtest.h>
#include <cobs/cortex.hpp>
#include <iostream>
#include <cobs/util/processing.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>


namespace {
    namespace fs = cobs::fs;

    std::string in_dir = "test/resources/cortex/input/";
    std::string out_dir = "test/out/cortex/";
    std::string in_path = in_dir + "sample.ctx";
    std::string out_path = out_dir + "sample.sam.isi";
    std::string out_path_rec = out_dir + "sample_rec.sam.isi";
    std::string sample_path = "test/resources/cortex/result/sample_sorted.txt";
    std::string sample_name = "DRR030535";

    template<unsigned int N>
    void assert_equals_sample(cobs::sample<N> sample) {
        std::ifstream ifs(sample_path);
        std::string line;
        size_t i = 0;
        while (std::getline(ifs, line)) {
            ASSERT_EQ(line, sample.data()[i].string());
            i++;
        }
        ASSERT_EQ(sample.data().size(), i);
    }

    TEST(cortex, file_name) {
        cobs::sample<31> sample1;
        cobs::sample<31> sample2;
        cobs::cortex::process_file(in_path, out_path, sample1);
        cobs::file::sample_header h;
        cobs::file::deserialize(out_path, sample2, h);
        ASSERT_EQ(h.name(), sample_name);
    }

    TEST(cortex, process_file) {
        fs::remove_all(out_dir);
        cobs::sample<31> sample;
        cobs::cortex::process_file(in_path, out_path, sample);
        assert_equals_sample(sample);
    }

    TEST(cortex, file_serialization) {
        fs::remove_all(out_dir);
        cobs::sample<31> sample1;
        cobs::sample<31> sample2;
        cobs::cortex::process_file(in_path, out_path, sample1);
        cobs::file::deserialize(out_path, sample2);
        assert_equals_sample(sample1);
        assert_equals_sample(sample2);
    }

    TEST(cortex, process_all_in_directory) {
        fs::remove_all(out_dir);
        cobs::cortex::process_all_in_directory<31>(in_dir, out_dir);
        cobs::sample<31> sample;
        cobs::file::deserialize(out_path_rec, sample);
        assert_equals_sample(sample);
    }
}
