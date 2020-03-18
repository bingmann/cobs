/*******************************************************************************
 * tests/file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <gtest/gtest.h>
#include <iostream>

#include <cobs/file/classic_index_header.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/header.hpp>
#include <cobs/kmer_buffer.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

namespace fs = cobs::fs;

TEST(file, base_name) {
    fs::path out_path("data/out/file/classic_index.cobs_doc");
    ASSERT_EQ("classic_index", cobs::base_name(out_path));
}

TEST(file, document) {
    std::stringstream buffer;

    cobs::KMerBuffer<31> doc_out;
    doc_out.serialize(buffer, "document");

    cobs::KMerBufferHeader hdoc;
    cobs::KMerBuffer<31> doc_in;
    doc_in.deserialize(buffer, hdoc);
}

TEST(file, classic_index_header) {
    std::stringstream buffer;

    // write classic index header
    std::vector<std::string> file_names = { "n1", "n2", "n3", "n4" };
    cobs::ClassicIndexHeader h_out;
    h_out.term_size_ = 31;
    h_out.canonicalize_ = 1;
    h_out.signature_size_ = 321;
    h_out.num_hashes_ = 21;
    h_out.file_names_ = file_names;
    h_out.serialize(buffer);

    // read classic index header
    cobs::ClassicIndexHeader h_in;
    h_in.deserialize(buffer);

    // compare results
    ASSERT_EQ(h_out.term_size_, h_in.term_size_);
    ASSERT_EQ(h_out.canonicalize_, h_in.canonicalize_);
    ASSERT_EQ(h_out.signature_size_, h_in.signature_size_);
    ASSERT_EQ(h_out.row_size(), h_in.row_size());
    ASSERT_EQ(h_out.num_hashes_, h_in.num_hashes_);
    ASSERT_EQ(file_names, h_in.file_names_);
}

TEST(file, classic_index) {
    std::stringstream buffer;

    // write classic index file
    std::vector<std::string> file_names = { "n1", "n2", "n3", "n4" };
    cobs::ClassicIndexHeader h_out;
    h_out.term_size_ = 31;
    h_out.canonicalize_ = 1;
    h_out.signature_size_ = 123;
    h_out.num_hashes_ = 12;
    h_out.file_names_ = file_names;
    std::vector<uint8_t> v_out(h_out.row_size() * h_out.signature_size_, 7);
    h_out.write_file(buffer, v_out);

    // read classic index file
    cobs::ClassicIndexHeader h_in;
    std::vector<uint8_t> v_in;
    h_in.read_file(buffer, v_in);

    // compare results
    ASSERT_EQ(h_out.term_size_, h_in.term_size_);
    ASSERT_EQ(h_out.canonicalize_, h_in.canonicalize_);
    ASSERT_EQ(h_out.signature_size_, h_in.signature_size_);
    ASSERT_EQ(h_out.row_size(), h_in.row_size());
    ASSERT_EQ(h_out.num_hashes_, h_in.num_hashes_);
    ASSERT_EQ(v_out, v_in);
    ASSERT_EQ(file_names, h_in.file_names_);
}

TEST(file, compact_index_header_values) {
    std::stringstream buffer;

    // write compact file header
    std::vector<cobs::CompactIndexHeader::parameter> parameters = {
        { 100, 1 },
        { 200, 1 },
        { 3000, 1 },
    };
    std::vector<std::string> file_names = { "file_1", "file_2", "file_3" };
    cobs::CompactIndexHeader h_out;
    h_out.term_size_ = 31;
    h_out.canonicalize_ = 1;
    h_out.parameters_ = parameters;
    h_out.file_names_ = file_names;
    h_out.page_size_ = 4096;
    h_out.serialize(buffer);

    // read compact file header
    cobs::CompactIndexHeader h_in;
    h_in.deserialize(buffer);

    // compare results
    for (size_t i = 0; i < parameters.size(); i++) {
        ASSERT_EQ(parameters[i].num_hashes, h_in.parameters_[i].num_hashes);
        ASSERT_EQ(parameters[i].signature_size, h_in.parameters_[i].signature_size);
    }
    ASSERT_EQ(file_names, h_in.file_names_);
}

TEST(file, compact_index_header_padding) {
    std::stringstream buffer;

    // write compact file header
    std::vector<cobs::CompactIndexHeader::parameter> parameters = { };
    std::vector<std::string> file_names = { };
    uint64_t page_size = 4096;
    cobs::CompactIndexHeader h_out;
    h_out.term_size_ = 31;
    h_out.canonicalize_ = 1;
    h_out.parameters_ = parameters;
    h_out.file_names_ = file_names;
    h_out.page_size_ = page_size;
    h_out.serialize(buffer);

    // read compact file header
    cobs::CompactIndexHeader h_in;
    h_in.deserialize(buffer);
    cobs::StreamPos sp = cobs::get_stream_pos(buffer);
    ASSERT_EQ(sp.curr_pos % page_size, 0U);
}

/******************************************************************************/
