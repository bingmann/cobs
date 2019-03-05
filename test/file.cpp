/*******************************************************************************
 * test/file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <gtest/gtest.h>
#include <iostream>

#include <cobs/document.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/header.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

namespace fs = cobs::fs;

TEST(file, base_name) {
    fs::path out_path("test/out/file/classic_index.doc.cobs");
    ASSERT_EQ("classic_index", cobs::base_name(out_path));
}

TEST(file, document) {
    std::stringstream buffer;

    cobs::Document<31> d_out;
    d_out.serialize(buffer, "document");

    cobs::DocumentHeader hdoc;
    cobs::Document<31> d_in;
    d_in.deserialize(buffer, hdoc);
}

TEST(file, classic_index_header) {
    std::stringstream buffer;

    // write classic index header
    std::vector<std::string> file_names = { "n1", "n2", "n3", "n4" };
    cobs::ClassicIndexHeader h_out(321, 21, file_names);
    h_out.serialize(buffer);

    // read classic index header
    cobs::ClassicIndexHeader h_in;
    h_in.deserialize(buffer);

    // compare results
    ASSERT_EQ(h_out.signature_size(), h_in.signature_size());
    ASSERT_EQ(h_out.block_size(), h_in.block_size());
    ASSERT_EQ(h_out.num_hashes(), h_in.num_hashes());
    ASSERT_EQ(file_names, h_in.file_names());
}

TEST(file, classic_index) {
    std::stringstream buffer;

    // write classic index file
    std::vector<std::string> file_names = { "n1", "n2", "n3", "n4" };
    cobs::ClassicIndexHeader h_out(123, 12, file_names);
    std::vector<uint8_t> v_out(h_out.block_size() * h_out.signature_size(), 7);
    h_out.write_file(buffer, v_out);

    // read classic index file
    cobs::ClassicIndexHeader h_in;
    std::vector<uint8_t> v_in;
    h_in.read_file(buffer, v_in);

    // compare results
    ASSERT_EQ(h_out.signature_size(), h_in.signature_size());
    ASSERT_EQ(h_out.block_size(), h_in.block_size());
    ASSERT_EQ(h_out.num_hashes(), h_in.num_hashes());
    ASSERT_EQ(v_out, v_in);
    ASSERT_EQ(file_names, h_in.file_names());
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
    cobs::CompactIndexHeader h_out(parameters, file_names, 4096);
    h_out.serialize(buffer);

    // read compact file header
    cobs::CompactIndexHeader h_in;
    h_in.deserialize(buffer);

    // compare results
    for (size_t i = 0; i < parameters.size(); i++) {
        ASSERT_EQ(parameters[i].num_hashes, h_in.parameters()[i].num_hashes);
        ASSERT_EQ(parameters[i].signature_size, h_in.parameters()[i].signature_size);
    }
    ASSERT_EQ(file_names, h_in.file_names());
}

TEST(file, compact_index_header_padding) {
    std::stringstream buffer;

    // write compact file header
    std::vector<cobs::CompactIndexHeader::parameter> parameters = { };
    std::vector<std::string> file_names = { };
    uint64_t page_size = 4096;
    cobs::CompactIndexHeader h_out(parameters, file_names, page_size);
    h_out.serialize(buffer);

    // read compact file header
    cobs::CompactIndexHeader h_in;
    h_in.deserialize(buffer);
    cobs::StreamPos sp = cobs::get_stream_pos(buffer);
    ASSERT_EQ(sp.curr_pos % page_size, 0U);
}

/******************************************************************************/
