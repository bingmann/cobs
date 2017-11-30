#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <gtest/gtest.h>
#include <isi/query/compact_index/mmap.hpp>
#include <isi/construction/compact_index.hpp>

inline void assert_equals_files(const std::string& f1, const std::string& f2) {
    std::ifstream ifs1(f1, std::ios::in | std::ios::binary);
    std::ifstream ifs2(f2, std::ios::in | std::ios::binary);
    std::istream_iterator<char> start1(ifs1);
    std::istream_iterator<char> start2(ifs2);
    std::istream_iterator<char> end;
    std::vector<char> v1(start1, end);
    std::vector<char> v2(start2, end);

    ASSERT_EQ(v1.size(), v2.size());
    for (size_t i = 0; i < v1.size(); i++) {
        ASSERT_EQ(v1[i], v2[i]);
    }
}

inline std::vector<isi::sample<31>> generate_samples_all(const std::string& query) {
    std::vector<isi::sample<31>> samples(33);
    isi::kmer<31> k;
    for (size_t i = 0; i < query.size() - 31; i++) {
        k.init(query.data() + i);
        for (size_t j = 0; j < samples.size(); j++) {
            if (j % (i % (samples.size() - 1) + 1) == 0) {
                samples[j].data().push_back(k);
            }
        }
    }
    return samples;
}

inline std::vector<isi::sample<31>> generate_samples_one(const std::string& query) {
    std::vector<isi::sample<31>> samples(33);
    isi::kmer<31> k;
    k.init(query.data());
    for (size_t i = 0; i < samples.size(); i++) {
        for (size_t j = 0; j < i * 10 + 1; j++) {
            samples[i].data().push_back(k);
        }
    }
    return samples;
}

inline std::string get_file_stem(size_t index) {
    assert(index < 100);
    std::string num = (index < 10 ? "0" : "") + std::to_string(index);
    return "sample_" + num;
}

inline void generate_test_case(std::vector<isi::sample<31>> samples, const std::string& out_dir) {
    for (size_t i = 0; i < samples.size(); i++) {
        std::string file_name = get_file_stem(i);
        isi::file::serialize(out_dir + "/" + file_name + isi::file::sample_header::file_extension, samples[i], file_name);
    }
}

