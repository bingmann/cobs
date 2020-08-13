/*******************************************************************************
 * tests/compact_index_query.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <gtest/gtest.h>
#ifdef __linux__
#include <cobs/query/compact_index/aio_search_file.hpp>
#endif

namespace fs = cobs::fs;

static fs::path base_dir = "data/compact_index_query";
static fs::path input_dir = base_dir / "input";
static fs::path index_file = base_dir / "index.cobs_compact";
static fs::path tmp_path = base_dir / "tmp";
static std::string query = cobs::random_sequence(21000, 1);

class compact_index_query : public ::testing::Test
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

TEST_F(compact_index_query, all_included_mmap_small) {
    // generate
    std::string query = cobs::random_sequence(160, 1);
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());

    // construct compact index and mmap query
    cobs::CompactIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.page_size = 2;
    index_params.canonicalize = 1;

    cobs::compact_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::CompactIndexMMapSearchFile>(index_file));

    // execute query and check results. check all expansion tables
    {
        // 8-bit non-SSE
        cobs::classic_search_disable_8bit = false;
        cobs::classic_search_disable_16bit = true;
        cobs::classic_search_disable_32bit = true;
        cobs::classic_search_disable_sse2 = true;

        std::vector<cobs::SearchResult> result;
        s_base.search(query, result);
        ASSERT_EQ(documents.size(), result.size());
        for (auto& r : result) {
            std::string doc_name = r.doc_name;
            int index = std::stoi(doc_name.substr(doc_name.size() - 2));
            ASSERT_GE(r.score, documents[index].data().size());
        }
    }
}

TEST_F(compact_index_query, all_included_mmap) {
    // generate
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());

    // construct compact index and mmap query
    cobs::CompactIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.page_size = 2;
    index_params.canonicalize = 1;

    cobs::compact_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::CompactIndexMMapSearchFile>(index_file));

    // execute query and check results. check all expansion tables
    {
        // 16-bit non-SSE
        cobs::classic_search_disable_8bit = true;
        cobs::classic_search_disable_16bit = false;
        cobs::classic_search_disable_32bit = true;
        cobs::classic_search_disable_sse2 = true;

        std::vector<cobs::SearchResult> result;
        s_base.search(query, result);
        ASSERT_EQ(documents.size(), result.size());
        for (auto& r : result) {
            std::string doc_name = r.doc_name;
            int index = std::stoi(doc_name.substr(doc_name.size() - 2));
            ASSERT_GE(r.score, documents[index].data().size());
        }
    }
    {
        // 16-bit SSE
        cobs::classic_search_disable_8bit = true;
        cobs::classic_search_disable_16bit = false;
        cobs::classic_search_disable_32bit = true;
        cobs::classic_search_disable_sse2 = false;

        std::vector<cobs::SearchResult> result;
        s_base.search(query, result);
        ASSERT_EQ(documents.size(), result.size());
        for (auto& r : result) {
            std::string doc_name = r.doc_name;
            int index = std::stoi(doc_name.substr(doc_name.size() - 2));
            ASSERT_GE(r.score, documents[index].data().size());
        }
    }
    {
        // 32-bit non-SSE
        cobs::classic_search_disable_8bit = true;
        cobs::classic_search_disable_16bit = true;
        cobs::classic_search_disable_32bit = false;
        cobs::classic_search_disable_sse2 = true;

        std::vector<cobs::SearchResult> result;
        s_base.search(query, result);
        ASSERT_EQ(documents.size(), result.size());
        for (auto& r : result) {
            std::string doc_name = r.doc_name;
            int index = std::stoi(doc_name.substr(doc_name.size() - 2));
            ASSERT_GE(r.score, documents[index].data().size());
        }
    }
    {
        // 32-bit SSE
        cobs::classic_search_disable_8bit = true;
        cobs::classic_search_disable_16bit = true;
        cobs::classic_search_disable_32bit = false;
        cobs::classic_search_disable_sse2 = false;

        std::vector<cobs::SearchResult> result;
        s_base.search(query, result);
        ASSERT_EQ(documents.size(), result.size());
        for (auto& r : result) {
            std::string doc_name = r.doc_name;
            int index = std::stoi(doc_name.substr(doc_name.size() - 2));
            ASSERT_GE(r.score, documents[index].data().size());
        }
    }
}

TEST_F(compact_index_query, one_included_mmap) {
    // generate
    auto documents = generate_documents_one(query, /* num_documents */ 2000);
    generate_test_case(documents, input_dir.string());

    // construct compact index and mmap query
    cobs::CompactIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.page_size = 2;
    index_params.canonicalize = 1;

    cobs::compact_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::CompactIndexMMapSearchFile>(index_file));

    // execute query and check results
    std::vector<cobs::SearchResult> result;
    s_base.search(query, result);
    ASSERT_EQ(documents.size(), result.size());
    for (size_t i = 0; i < result.size(); ++i) {
        ASSERT_EQ(result[i].score, 1);
    }
}

TEST_F(compact_index_query, false_positive_mmap) {
    // generate
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());

    // construct compact index and mmap query
    cobs::CompactIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.page_size = 2;
    index_params.canonicalize = 1;

    cobs::compact_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::CompactIndexMMapSearchFile>(index_file));

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

#if COBS_USE_AIO_CURRENTLY_DISABLED

TEST_F(compact_index_query, all_included_aio) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());
    cobs::compact_construct_from_documents(input_dir, index_dir, 8, 3, 0.1, 4096);
    cobs::CompactIndexAioSearchFile s_aio(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_aio.search(query, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        int index = std::stoi(r.second.substr(r.second.size() - 2));
        ASSERT_GE(r.first, documents[index].data().size());
    }
}

TEST_F(compact_index_query, one_included_aio) {
    auto documents = generate_documents_one(query);
    generate_test_case(documents, input_dir.string());
    cobs::compact_construct_from_documents(input_dir, index_dir, 8, 3, 0.1, 4096);
    cobs::CompactIndexAioSearchFile s_aio(index_path);

    std::vector<std::pair<uint16_t, std::string> > result;
    s_aio.search(query, result);
    ASSERT_EQ(documents.size(), result.size());
    for (auto& r : result) {
        ASSERT_EQ(r.first, 1);
    }
}

TEST_F(compact_index_query, false_positive_aio) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());
    cobs::compact_construct_from_documents(input_dir, index_dir, 8, 3, 0.1, 4096);
    cobs::CompactIndexAioSearchFile s_aio(index_path);

    size_t num_tests = 10000;
    std::map<std::string, uint64_t> num_positive;
    std::vector<std::pair<uint16_t, const char*> > result;
    for (size_t i = 0; i < num_tests; i++) {
        std::string query_2 = cobs::random_sequence(31, i);
        s_aio.search(query_2, result);

        for (auto& r : result) {
            num_positive[r.second] += r.first;
            ASSERT_TRUE(r.first == 0 || r.first == 1);
        }
    }

    for (auto& np : num_positive) {
        ASSERT_LE(np.second, 1070U);
    }
}

#endif // COBS_USE_AIO_CURRENTLY_DISABLED

/******************************************************************************/
