/*******************************************************************************
 * test/src/frequency.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

#include <cobs/frequency.hpp>
#include <cobs/util/fs.hpp>

namespace {
namespace fs = cobs::fs;

fs::path in_dir("test/resources/frequency/input/");
fs::path out_dir("test/out/frequency/");
fs::path result_bin("test/resources/frequency/result/bin.freq.isi");
fs::path result_freq("test/resources/frequency/result/freq.freq.isi");
fs::path document_1(in_dir.string() + "document_1.doc.isi");
fs::path document_2(in_dir.string() + "document_2.doc.isi");
fs::path document_3(in_dir.string() + "document_3.doc.isi");

/*
std::string document_4 = in_dir + "document_4.freq.cobs";
std::string document_5 = in_dir + "document_5.freq.cobs";
void generate_result_bin() {
    fs::create_directories(out_dir);
    std::vector<uint64_t> v;
    document<31> s;
    file::deserialize(document_1, s);
    v.insert(v.end(), reinterpret_cast<uint64_t*>(&(*s.data().begin())), reinterpret_cast<uint64_t*>(&(*s.data().end())));
    file::deserialize(document_2, s);
    v.insert(v.end(), reinterpret_cast<uint64_t*>(&(*s.data().begin())), reinterpret_cast<uint64_t*>(&(*s.data().end())));
    file::deserialize(document_3, s);
    v.insert(v.end(), reinterpret_cast<uint64_t*>(&(*s.data().begin())), reinterpret_cast<uint64_t*>(&(*s.data().end())));
    std::sort(v.begin(), v.end());

    std::ofstream ofs;
    file::serialize_header<file::frequency_header>(ofs, "bin" + file::frequency_header::file_extension, file::frequency_header());
    uint64_t kmer = v[0];
    uint32_t count = 1;
    for(size_t i = 1; i < v.size(); i++) {
        if (kmer == v[i]) {
            count++;
        } else {
            ofs.write(reinterpret_cast<const char*>(&kmer), 8);
            ofs.write(reinterpret_cast<const char*>(&count), 4);
            kmer = v[i];
            count = 1;
        }
    }
    ofs.write(reinterpret_cast<const char*>(&kmer), 8);
    ofs.write(reinterpret_cast<const char*>(&count), 4);
}

template<class OutputIteratorKmer, class OutputIteratorCount>
void read_document(const std::string& file, OutputIteratorKmer& iter_kmer, OutputIteratorCount& iter_count) {
    std::ifstream ifs;
    file::deserialize_header<file::frequency_header>(ifs, file);
    while(ifs && ifs.peek() != EOF) {
        uint64_t kmer;
        uint32_t count;
        ifs.read(reinterpret_cast<char*>(&kmer), 8);
        ifs.read(reinterpret_cast<char*>(&count), 4);
        *iter_kmer++ = kmer;
        *iter_count++ = count;
    }
}

void generate_result_fre() {
    fs::create_directories(out_dir);
    std::vector<uint64_t> kmers;
    std::vector<uint32_t> counts;
    std::back_insert_iterator<std::vector<uint64_t>> iter_kmer(kmers);
    std::back_insert_iterator<std::vector<uint32_t >> iter_count(counts);
    read_document(document_4, iter_kmer, iter_count);
    read_document(document_5, iter_kmer, iter_count);
    std::vector<size_t> indices(kmers.size());
    for (size_t i = 0; i < kmers.size(); i++) {
        indices[i] = i;
    }
    std::sort(indices.begin(), indices.end(), [&](const auto& i1, const auto& i2) {
        return kmers[i1] < kmers[i2];
    });

    std::ofstream ofs;
    file::serialize_header<file::frequency_header>(ofs, "freq" + file::frequency_header::file_extension, file::frequency_header());
    uint64_t kmer = kmers[indices[0]];
    uint32_t count = counts[indices[0]];
    for(size_t i = 1; i < kmers.size(); i++) {
        if (kmer == kmers[indices[i]]) {
            count += counts[indices[i]];
        } else {
            ofs.write(reinterpret_cast<const char*>(&kmer), 8);
            ofs.write(reinterpret_cast<const char*>(&count), 4);
            kmer = kmers[indices[i]];
            count = counts[indices[i]];
        }
    }
    ofs.write(reinterpret_cast<const char*>(&kmer), 8);
    ofs.write(reinterpret_cast<const char*>(&count), 4);
}

TEST(frequency, generate_results) {
    generate_result_bin();
    generate_result_freq();
}
*/

size_t get_count_document(const fs::path& file) {
    std::ifstream ifs;
    cobs::file::deserialize_header<cobs::file::document_header>(ifs, file);
    ifs.exceptions(std::ios::failbit | std::ios::badbit);
    size_t total_count = 0;
    uint64_t kmer;
    while (ifs && ifs.peek() != EOF) {
        ifs.read(reinterpret_cast<char*>(&kmer), 8);
        total_count++;
    }
    return total_count;
}

size_t get_count_frequency(const fs::path& file) {
    std::ifstream ifs;
    cobs::file::deserialize_header<cobs::file::frequency_header>(ifs, file);
    ifs.exceptions(std::ios::failbit | std::ios::badbit);
    size_t total_count = 0;
    uint64_t kmer;
    uint32_t count;
    while (ifs && ifs.peek() != EOF) {
        ifs.read(reinterpret_cast<char*>(&kmer), 8);
        ifs.read(reinterpret_cast<char*>(&count), 4);
        total_count += count;
    }
    return total_count;
}

class frequency : public ::testing::Test
{
protected:
    virtual void SetUp() {

        cobs::error_code ec;
        fs::remove_all(out_dir, ec);
    }
};

TEST_F(frequency, bin) {
    cobs::frequency::process_all_in_directory<cobs::file::document_header>(in_dir, out_dir, 40);
    size_t total_count = get_count_document(document_1) + get_count_document(document_2) + get_count_document(document_3);
    fs::path p(out_dir.string() + "[document_1-document_3]" + cobs::file::frequency_header::file_extension);
    ASSERT_EQ(get_count_frequency(p), total_count);
    assert_equals_files(result_bin.string(), p.string());
}

TEST_F(frequency, freq) {
    cobs::frequency::process_all_in_directory<cobs::file::frequency_header>(in_dir, out_dir, 40);
    fs::path p(out_dir.string() + "[document_4-document_5]" + cobs::file::frequency_header::file_extension);
    assert_equals_files(result_freq.string(), p.string());
}
}

/******************************************************************************/
