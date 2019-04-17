/*******************************************************************************
 * tests/classic_index_construction.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path input_dir("test/classic_index_construction/input");
static fs::path index_dir("test/classic_index_construction/index");
static fs::path classic_index_path(index_dir.string() + "/index.cobs_classic");

class classic_index_construction : public ::testing::Test
{
protected:
    void SetUp() final {
        cobs::error_code ec;
        fs::remove_all(index_dir, ec);
        fs::remove_all(input_dir, ec);
    }
    void TearDown() final {
        cobs::error_code ec;
        fs::remove_all(input_dir, ec);
        fs::remove_all(index_dir, ec);
    }
};

TEST_F(classic_index_construction, deserialization) {
    // generate
    std::string query = cobs::random_sequence(10000, 1);
    auto documents = generate_documents_all(query, /* num_documents */ 33);
    generate_test_case(documents, input_dir.string());

    // get file names
    std::vector<fs::path> paths;
    std::copy_if(fs::recursive_directory_iterator(input_dir),
                 fs::recursive_directory_iterator(),
                 std::back_inserter(paths),
                 [](const auto& p) {
                     return cobs::file_has_header<cobs::KMerBufferHeader>(p);
                 });
    std::sort(paths.begin(), paths.end());

    // construct classic index
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;

    cobs::classic_construct(input_dir, index_dir, index_params);

    // read classic index and check header fields
    std::vector<uint8_t> data;
    cobs::ClassicIndexHeader h;
    h.read_file(classic_index_path, data);
    ASSERT_EQ(h.file_names().size(), 33u);
    ASSERT_EQ(h.num_hashes(), 3u);
    ASSERT_EQ(h.file_names().size(), paths.size());
    for (size_t i = 0; i < h.file_names().size(); i++) {
        ASSERT_EQ(h.file_names()[i], cobs::base_name(paths[i]));
    }

    // check ratio of zeros/ones
    std::map<std::string, size_t> num_ones;
    for (size_t j = 0; j < h.signature_size(); j++) {
        for (size_t k = 0; k < h.row_size(); k++) {
            uint8_t d = data[j * h.row_size() + k];
            for (size_t o = 0; o < 8; o++) {
                size_t file_names_index = k * 8 + o;
                if (file_names_index < h.file_names().size()) {
                    std::string file_name = h.file_names()[file_names_index];
                    num_ones[file_name] += (d & (1 << o)) >> o;
                }
            }
        }
    }

    double set_bit_ratio =
        cobs::calc_average_set_bit_ratio(h.signature_size(), 3, 0.1);
    double num_ones_average = set_bit_ratio * h.signature_size();
    for (auto& no : num_ones) {
        ASSERT_LE(no.second, num_ones_average * 1.01);
    }
}

/******************************************************************************/
