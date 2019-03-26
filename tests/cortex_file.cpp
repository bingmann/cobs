/*******************************************************************************
 * tests/cortex_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/cortex_file.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/processing.hpp>
#include <gtest/gtest.h>
#include <iostream>

namespace fs = cobs::fs;

static std::string in_dir = "test/resources/cortex/input/";
static std::string out_dir = "test/out/cortex/";
static std::string in_path = in_dir + "document.ctx";
static std::string out_path = out_dir + "document.cobs_doc";
static std::string out_path_rec = out_dir + "document_rec.cobs_doc";
static std::string document_path = "test/resources/cortex/result/document_sorted.txt";
static std::string document_name = "DRR030535";

class cortex : public ::testing::Test
{
protected:
    void SetUp() final {
        cobs::error_code ec;
        fs::remove_all(out_dir, ec);
    }
    void TearDown() final {
        cobs::error_code ec;
        fs::remove_all(out_dir, ec);
    }
};

template <unsigned int N>
void assert_equals_document(cobs::Document<N> document) {
    std::ifstream ifs(document_path);
    std::string line;
    size_t i = 0;
    while (std::getline(ifs, line)) {
        ASSERT_EQ(line, document.data()[i].string());
        i++;
    }
    ASSERT_EQ(document.data().size(), i);
}

TEST_F(cortex, file_name) {
    cobs::Document<31> document1;
    cobs::Document<31> document2;
    cobs::Timer t;
    cobs::process_file(in_path, out_path, document1, t);
    cobs::DocumentHeader h;
    document2.deserialize(out_path, h);
    ASSERT_EQ(h.name(), document_name);
}

TEST_F(cortex, process_file) {
    cobs::Document<31> document;
    cobs::Timer t;
    cobs::process_file(in_path, out_path, document, t);
    document.sort_kmers();
    assert_equals_document(document);
}

TEST_F(cortex, file_serialization) {
    cobs::Document<31> document1;
    cobs::Document<31> document2;
    cobs::Timer t;
    cobs::process_file(in_path, out_path, document1, t);
    cobs::DocumentHeader hdoc;
    document2.deserialize(out_path, hdoc);
    document1.sort_kmers();
    document2.sort_kmers();
    assert_equals_document(document1);
    assert_equals_document(document2);
}

TEST_F(cortex, process_all_in_directory) {
    cobs::process_all_in_directory<31>(in_dir, out_dir);
    cobs::DocumentHeader hdoc;
    cobs::Document<31> document;
    document.deserialize(out_path_rec, hdoc);
    document.sort_kmers();
    assert_equals_document(document);
}

TEST_F(cortex, process_kmers) {
    cobs::CortexFile ctx(in_path);

    // check contents
    die_unequal(ctx.version_, 6u);
    die_unequal(ctx.kmer_size_, 31u);
    die_unequal(ctx.num_words_per_kmer_, 1u);
    die_unequal(ctx.num_colors_, 1u);
    die_unequal(ctx.name_, "DRR030535");

    die_unequal(ctx.num_kmers(), 24158u);

    // process kmers
    size_t count = 0;
    ctx.process_kmers<31>([&](cobs::KMer<31>&) { ++count; });

    die_unequal(ctx.num_kmers(), count);
}

/******************************************************************************/
