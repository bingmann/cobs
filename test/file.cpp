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

static fs::path out_dir("test/out/file");
static fs::path out_path_s(out_dir.string() + "/classic_index.doc.isi");
static fs::path out_path_isi(out_dir.string() + "/classic_index.cla_idx.isi");
static fs::path out_path_cisi(out_dir.string() + "/compact_index.com_idx.isi");

class file : public ::testing::Test
{
protected:
    virtual void SetUp() {
        cobs::error_code ec;
        fs::remove_all(out_dir, ec);
        fs::create_directories(out_dir);
    }
};

TEST_F(file, document) {
    cobs::Document<31> s_out;
    s_out.serialize(out_path_s, "document");

    cobs::file::document_header hdoc;
    cobs::Document<31> s_in;
    s_in.deserialize(out_path_s, hdoc);
}

TEST_F(file, classic_index) {
    std::vector<std::string> file_names = { "n1", "n2", "n3", "n4" };
    cobs::file::classic_index_header h_out(123, 12, file_names);
    std::vector<uint8_t> v_out(h_out.block_size() * h_out.signature_size(), 7);
    h_out.write_file(out_path_isi, v_out);

    cobs::file::classic_index_header h_in;
    std::vector<uint8_t> v_in;
    h_in.read_file(out_path_isi, v_in);
    ASSERT_EQ(h_out.signature_size(), h_in.signature_size());
    ASSERT_EQ(h_out.block_size(), h_in.block_size());
    ASSERT_EQ(h_out.num_hashes(), h_in.num_hashes());
    ASSERT_EQ(v_out.size(), v_in.size());
    for (size_t i = 0; i < v_out.size(); i++) {
        ASSERT_EQ(v_out[i], v_in[i]);
    }
    ASSERT_EQ(file_names.size(), h_in.file_names().size());
    for (size_t i = 0; i < file_names.size(); i++) {
        ASSERT_EQ(file_names[i], h_in.file_names()[i]);
    }
}

TEST_F(file, classic_index_header) {
    std::vector<std::string> file_names = { "n1", "n2", "n3", "n4" };
    cobs::file::classic_index_header h_out(321, 21, file_names);
    cobs::file::serialize_header(out_path_isi, h_out);

    auto h_in = cobs::file::deserialize_header<cobs::file::classic_index_header>(out_path_isi);
    ASSERT_EQ(h_out.signature_size(), h_in.signature_size());
    ASSERT_EQ(h_out.block_size(), h_in.block_size());
    ASSERT_EQ(h_out.num_hashes(), h_in.num_hashes());
    ASSERT_EQ(file_names.size(), h_in.file_names().size());
    for (size_t i = 0; i < file_names.size(); i++) {
        ASSERT_EQ(file_names[i], h_in.file_names()[i]);
    }
}

TEST_F(file, compact_index_header_values) {
    std::vector<cobs::file::compact_index_header::parameter> parameters = {
        { 100, 1 },
        { 200, 1 },
        { 3000, 1 },
    };
    std::vector<std::string> file_names = { "file_1", "file_2", "file_3" };
    cobs::file::compact_index_header h(parameters, file_names, 4096);
    cobs::file::serialize_header(out_path_cisi, h);

    auto h_2 = cobs::file::deserialize_header<cobs::file::compact_index_header>(out_path_cisi);

    for (size_t i = 0; i < parameters.size(); i++) {
        ASSERT_EQ(parameters[i].num_hashes, h_2.parameters()[i].num_hashes);
        ASSERT_EQ(parameters[i].signature_size, h_2.parameters()[i].signature_size);
    }
    for (size_t i = 0; i < file_names.size(); i++) {
        ASSERT_EQ(file_names[i], h_2.file_names()[i]);
    }
}

TEST_F(file, compact_index_header_padding) {
    std::vector<cobs::file::compact_index_header::parameter> parameters = { };
    std::vector<std::string> file_names = { };
    uint64_t page_size = 4096;
    cobs::file::compact_index_header h(parameters, file_names, page_size);
    cobs::file::serialize_header(out_path_cisi, h);

    std::ifstream ifs;
    cobs::file::deserialize_header<cobs::file::compact_index_header>(ifs, out_path_cisi);
    cobs::stream_metadata smd = cobs::get_stream_metadata(ifs);
    ASSERT_EQ(smd.curr_pos % page_size, 0U);
}

/******************************************************************************/
