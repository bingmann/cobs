/*******************************************************************************
 * test/src/cortex.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/cortex.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/processing.hpp>
#include <gtest/gtest.h>
#include <iostream>

namespace {
namespace fs = cobs::fs;

std::string in_dir = "test/resources/cortex/input/";
std::string out_dir = "test/out/cortex/";
std::string in_path = in_dir + "document.ctx";
std::string out_path = out_dir + "document.doc.isi";
std::string out_path_rec = out_dir + "document_rec.doc.isi";
std::string document_path = "test/resources/cortex/result/document_sorted.txt";
std::string document_name = "DRR030535";

template <unsigned int N>
void assert_equals_document(cobs::document<N> document) {
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
    cobs::document<31> document1;
    cobs::document<31> document2;
    cobs::timer t;
    cobs::cortex::process_file(in_path, out_path, document1, t);
    cobs::file::document_header h;
    cobs::file::deserialize(out_path, document2, h);
    ASSERT_EQ(h.name(), document_name);
}

TEST(cortex, process_file) {
    fs::remove_all(out_dir);
    cobs::document<31> document;
    cobs::timer t;
    cobs::cortex::process_file(in_path, out_path, document, t);
    document.sort_kmers();
    assert_equals_document(document);
}

TEST(cortex, file_serialization) {
    fs::remove_all(out_dir);
    cobs::document<31> document1;
    cobs::document<31> document2;
    cobs::timer t;
    cobs::cortex::process_file(in_path, out_path, document1, t);
    cobs::file::deserialize(out_path, document2);
    document1.sort_kmers();
    document2.sort_kmers();
    assert_equals_document(document1);
    assert_equals_document(document2);
}

TEST(cortex, process_all_in_directory) {
    fs::remove_all(out_dir);
    cobs::cortex::process_all_in_directory<31>(in_dir, out_dir);
    cobs::document<31> document;
    cobs::file::deserialize(out_path_rec, document);
    document.sort_kmers();
    assert_equals_document(document);
}
}

/******************************************************************************/
