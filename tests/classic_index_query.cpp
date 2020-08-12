/*******************************************************************************
 * tests/classic_index_query.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/util/calc_signature_size.hpp>
#include <gtest/gtest.h>
#include <iostream>

namespace fs = cobs::fs;

static fs::path base_dir = "data/classic_index_query";
static fs::path input_dir = base_dir / "input";
static fs::path index_path = base_dir / "index.cobs_classic";
static fs::path tmp_path = base_dir / "tmp";
static std::string query = cobs::random_sequence(50000, 2);

class classic_index_query : public ::testing::Test
{
protected:
    void SetUp() final {
        cobs::error_code ec;
        fs::remove_all(base_dir, ec);
    }
    void TearDown() final {
        cobs::error_code ec;
        fs::remove_all(base_dir, ec);
    }
};

TEST_F(classic_index_query, all_included_small_batch) {
    // generate
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());

    // construct classic index and mmap query
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.canonicalize = 1;

    cobs::classic_construct(
        cobs::DocumentList(input_dir), index_path, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(index_path));

    // execute query and check results
    std::vector<cobs::SearchResult> result;
    s_base.search(query, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        std::string doc = r.doc_name;
        int index = std::stoi(doc.substr(doc.size() - 2));
        ASSERT_GE(r.score, documents[index].data().size());
    }
}

TEST_F(classic_index_query, one_included_small_batch) {
    // generate
    auto documents = generate_documents_one(query, /* num_documents */ 2000);
    generate_test_case(documents, input_dir.string());

    // construct classic index and mmap query
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.canonicalize = 1;

    cobs::classic_construct(
        cobs::DocumentList(input_dir), index_path, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(index_path));

    // execute query and check results
    std::vector<cobs::SearchResult> result;
    s_base.search(query, result);
    ASSERT_EQ(documents.size(), result.size());
    for (size_t i = 0; i < result.size(); ++i) {
        ASSERT_EQ(result[i].score, 1);
    }
}

TEST_F(classic_index_query, one_included_large_batch) {
    // generate
    auto documents = generate_documents_one(query);
    generate_test_case(documents, input_dir.string());

    // construct classic index and mmap query
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.canonicalize = 1;

    cobs::classic_construct(
        cobs::DocumentList(input_dir), index_path, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(index_path));

    // execute query and check results
    std::vector<cobs::SearchResult> result;
    s_base.search(query, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        ASSERT_EQ(r.score, 1);
    }
}

TEST_F(classic_index_query, false_positive) {
    // generate
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());

    // construct classic index and mmap query
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.canonicalize = 1;

    cobs::classic_construct(
        cobs::DocumentList(input_dir), index_path, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(index_path));

    // execute query and check results
    size_t num_tests = 10000;
    std::map<std::string, uint64_t> num_positive;
    std::vector<cobs::SearchResult> result;
    for (size_t i = 0; i < num_tests; i++) {
        std::string query_2 = cobs::random_sequence(31, i);
        s_base.search(query_2, result);

        for (auto& r : result) {
            num_positive[r.doc_name] += r.score;
            ASSERT_TRUE(r.score == 0 || r.score == 1);
        }
    }

    for (auto& np : num_positive) {
        ASSERT_LE(np.second, 1070U);
    }
}

static fs::path input1_dir = base_dir / "input1";
static fs::path input2_dir = base_dir / "input2";
static fs::path input3_dir = base_dir / "input3";

static fs::path index1_path = base_dir / "index1.cobs_classic";
static fs::path index2_path = base_dir / "index2.cobs_classic";
static fs::path index3_path = base_dir / "index3.cobs_classic";

TEST_F(classic_index_query, one_included_large_batch_multi_index) {
    // construct classic index and mmap query
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.canonicalize = 1;

    // generate index 1
    auto documents1 = generate_documents_one(query, /* documents */ 33);
    generate_test_case(documents1, input1_dir.string());

    cobs::classic_construct(
        cobs::DocumentList(input1_dir), index1_path, tmp_path, index_params);

    // generate index 2
    auto documents2 = generate_documents_one(query, /* documents */ 44);
    generate_test_case(documents2, input2_dir.string());

    cobs::classic_construct(
        cobs::DocumentList(input2_dir), index2_path, tmp_path, index_params);

    // generate index 3
    auto documents3 = generate_documents_one(query, /* documents */ 55);
    generate_test_case(documents3, input3_dir.string());

    cobs::classic_construct(
        cobs::DocumentList(input3_dir), index3_path, tmp_path, index_params);

    auto index1 = std::make_shared<cobs::ClassicIndexMMapSearchFile>(index1_path);
    auto index2 = std::make_shared<cobs::ClassicIndexMMapSearchFile>(index2_path);
    auto index3 = std::make_shared<cobs::ClassicIndexMMapSearchFile>(index3_path);

    cobs::ClassicSearch s_base({ index1, index2, index3 });

    // execute query and check results
    std::vector<cobs::SearchResult> result;
    s_base.search(query, result);
    ASSERT_EQ(33u + 44u + 55u, result.size());
    for (auto& r : result) {
        ASSERT_EQ(r.score, 1u);
    }
}

/******************************************************************************/
