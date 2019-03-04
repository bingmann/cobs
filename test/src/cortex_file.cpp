/*******************************************************************************
 * test/src/cortex_file.cpp
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
static std::string out_path = out_dir + "document.doc.isi";
static std::string out_path_rec = out_dir + "document_rec.doc.isi";
static std::string document_path = "test/resources/cortex/result/document_sorted.txt";
static std::string document_name = "DRR030535";

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

TEST(cortex, file_name) {
    cobs::Document<31> document1;
    cobs::Document<31> document2;
    cobs::Timer t;
    cobs::process_file(in_path, out_path, document1, t);
    cobs::file::document_header h;
    document2.deserialize(out_path, h);
    ASSERT_EQ(h.name(), document_name);
}

TEST(cortex, process_file) {
    fs::remove_all(out_dir);
    cobs::Document<31> document;
    cobs::Timer t;
    cobs::process_file(in_path, out_path, document, t);
    document.sort_kmers();
    assert_equals_document(document);
}

TEST(cortex, file_serialization) {
    fs::remove_all(out_dir);
    cobs::Document<31> document1;
    cobs::Document<31> document2;
    cobs::Timer t;
    cobs::process_file(in_path, out_path, document1, t);
    cobs::file::document_header hdoc;
    document2.deserialize(out_path, hdoc);
    document1.sort_kmers();
    document2.sort_kmers();
    assert_equals_document(document1);
    assert_equals_document(document2);
}

TEST(cortex, process_all_in_directory) {
    fs::remove_all(out_dir);
    cobs::process_all_in_directory<31>(in_dir, out_dir);
    cobs::file::document_header hdoc;
    cobs::Document<31> document;
    document.deserialize(out_path_rec, hdoc);
    document.sort_kmers();
    assert_equals_document(document);
}

/******************************************************************************/
