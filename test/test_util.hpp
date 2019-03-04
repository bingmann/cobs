/*******************************************************************************
 * test/test_util.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_TEST_TEST_UTIL_HEADER
#define COBS_TEST_TEST_UTIL_HEADER

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/compact_index.hpp>
#include <cobs/document.hpp>
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/compact_index/mmap.hpp>
#include <cobs/util/query.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

static inline
void assert_equals_files(const std::string& f1, const std::string& f2) {
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

static inline
std::vector<cobs::Document<31> > generate_documents_all(const std::string& query) {
    std::vector<cobs::Document<31> > documents(33);
    cobs::KMer<31> k;
    char kmer_buffer[31];
    for (size_t i = 0; i < query.size() - 31; i++) {
        const char* normalized_kmer =
            cobs::canonicalize_kmer(query.data() + i, kmer_buffer, 31);
        k.init(normalized_kmer);
        for (size_t j = 0; j < documents.size(); j++) {
            if (j % (i % (documents.size() - 1) + 1) == 0) {
                documents[j].data().push_back(k);
            }
        }
    }
    return documents;
}

static inline
std::vector<cobs::Document<31> > generate_documents_one(const std::string& query) {
    std::vector<cobs::Document<31> > documents(33);
    cobs::KMer<31> k;
    char kmer_buffer[31];
    const char* normalized_kmer =
        cobs::canonicalize_kmer(query.data(), kmer_buffer, 31);
    k.init(normalized_kmer);
    for (size_t i = 0; i < documents.size(); i++) {
        for (size_t j = 0; j < i * 10 + 1; j++) {
            documents[i].data().push_back(k);
        }
    }
    return documents;
}

static inline
std::string get_file_stem(size_t index) {
    assert(index < 100);
    std::string num = (index < 10 ? "0" : "") + std::to_string(index);
    return "document_" + num;
}

static inline
void generate_test_case(std::vector<cobs::Document<31> > documents,
                        const std::string& out_dir) {
    for (size_t i = 0; i < documents.size(); i++) {
        std::string file_name = get_file_stem(i);
        documents[i].serialize(
            out_dir + "/" + file_name + cobs::DocumentHeader::file_extension,
            file_name);
    }
}

#endif // !COBS_TEST_TEST_UTIL_HEADER

/******************************************************************************/
