/*******************************************************************************
 * test/classic_index_query.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/util/parameters.hpp>
#include <gtest/gtest.h>
#include <iostream>

namespace fs = cobs::fs;

static fs::path in_dir("test/out/classic_index_query/input");
static fs::path tmp_dir("test/out/classic_index_query/tmp");
static fs::path index_path(in_dir.string() + "/index.cla_idx.isi");
static std::string query = cobs::random_sequence(50000, 2);

class classic_index_query : public ::testing::Test
{
protected:
    virtual void SetUp() {
        cobs::error_code ec;
        fs::remove_all(in_dir, ec);
        if (ec) {
            std::cout << ec.message() << std::endl;
        }
        fs::remove_all(tmp_dir, ec);
        if (ec) {
            std::cout << ec.message() << std::endl;
        }
        fs::create_directories(in_dir);
        fs::create_directories(tmp_dir);
    }
};

TEST_F(classic_index_query, all_included_small_batch) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 16, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_mmap.search(query, 31, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        int index = std::stoi(r.second.substr(r.second.size() - 2));
        ASSERT_GE(r.first, documents[index].data().size());
    }
}

TEST_F(classic_index_query, all_included_large_batch) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 16, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_mmap.search(query, 31, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        int index = std::stoi(r.second.substr(r.second.size() - 2));
        ASSERT_GE(r.first, documents[index].data().size());
    }
}

TEST_F(classic_index_query, all_included_max_batch) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 16, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_mmap.search(query, 31, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        int index = std::stoi(r.second.substr(r.second.size() - 2));
        ASSERT_GE(r.first, documents[index].data().size());
    }
}

TEST_F(classic_index_query, one_included_small_batch) {
    auto documents = generate_documents_one(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 32, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_mmap.search(query, 31, result);
    std::vector<std::string> split;
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        ASSERT_EQ(r.first, 1);
    }
}

TEST_F(classic_index_query, one_included_large_batch) {
    auto documents = generate_documents_one(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 8, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_mmap.search(query, 31, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        ASSERT_EQ(r.first, 1);
    }
}

TEST_F(classic_index_query, one_included_max_batch) {
    auto documents = generate_documents_one(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 32, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_mmap.search(query, 31, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        ASSERT_EQ(r.first, 1);
    }
}

TEST_F(classic_index_query, false_positive) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 32, 3, 0.1);
    cobs::query::classic_index::mmap s_mmap(index_path);

    size_t num_tests = 10000;
    std::map<std::string, uint64_t> num_positive;
    std::vector<std::pair<uint16_t, std::string> > result;
    for (size_t i = 0; i < num_tests; i++) {
        std::string query_2 = cobs::random_sequence(31, i);
        s_mmap.search(query_2, 31, result);

        for (auto& r : result) {
            num_positive[r.second] += r.first;
            ASSERT_TRUE(r.first == 0 || r.first == 1);
        }
    }

    for (auto& np : num_positive) {
        ASSERT_LE(np.second, 1070U);
    }
}

/******************************************************************************/
