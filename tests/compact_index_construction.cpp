/*******************************************************************************
 * tests/compact_index_construction.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/settings.hpp>
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path base_dir = "data/compact_index_construction";
static fs::path input_dir = base_dir / "input";
static fs::path index_file = base_dir / "index.cobs_compact";
static fs::path tmp_path = base_dir / "tmp";
static fs::path cobs_2_dir = tmp_path / cobs::pad_index(2);

static std::string query = cobs::random_sequence(100000, 1);

class compact_index_construction : public ::testing::Test
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

TEST_F(compact_index_construction, padding) {
    // generate
    auto documents = generate_documents_all(query, /* num_documents */ 200);
    generate_test_case(documents, input_dir.string());

    // construct compact index
    cobs::CompactIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.page_size = 16;

    cobs::compact_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);

    // read compact index header, check page_size alignment of data
    std::ifstream ifs;
    cobs::deserialize_header<cobs::CompactIndexHeader>(ifs, index_file);
    cobs::StreamPos sp = cobs::get_stream_pos(ifs);
    ASSERT_EQ(sp.curr_pos % index_params.page_size, 0U);
}

TEST_F(compact_index_construction, deserialization) {
    // generate
    auto documents = generate_documents_all(query);
    generate_test_case(documents, input_dir.string());

    // get file names
    cobs::DocumentList doc_list(input_dir, cobs::FileType::Any);
    doc_list.sort_by_size();

    std::vector<cobs::DocumentEntry> paths = doc_list.list();
    for (size_t i = 0; i < documents.size(); i += 2 * 8) {
        size_t middle_index = std::min(i + 16, paths.size());
        std::sort(paths.begin() + i, paths.begin() + middle_index);
    }

    // construct compact index
    cobs::CompactIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.page_size = 2;
    index_params.keep_temporary = true;

    cobs::compact_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);

    // read compact index header and check fields
    std::vector<std::vector<uint8_t> > data;
    cobs::CompactIndexHeader h;
    h.read_file(index_file, data);
    ASSERT_EQ(h.file_names_.size(), 33U);
    ASSERT_EQ(h.parameters_.size(), 3U);
    ASSERT_EQ(data.size(), 3U);
    for (size_t i = 0; i < h.file_names_.size(); i++) {
        ASSERT_EQ(h.file_names_[i], cobs::base_name(paths[i].path_));
    }

    // check compact index parameters
    std::vector<uint64_t> document_sizes;
    std::vector<cobs::CompactIndexHeader::parameter> parameters;
    for (const fs::path& p : fs::recursive_directory_iterator(input_dir)) {
        // TODO: this test does nothing, because DocumentHeader should be below!
        if (cobs::file_has_header<cobs::ClassicIndexHeader>(p)) {
            std::cout << "doc: " << p.string() << std::endl;
            document_sizes.push_back(fs::file_size(p));
        }
    }

    std::sort(document_sizes.begin(), document_sizes.end());
    for (size_t i = 0; i < document_sizes.size(); i++) {
        if (i % 8 == 7) {
            uint64_t signature_size = cobs::calc_signature_size(
                document_sizes[i] / 8, index_params.num_hashes, 0.1);
            ASSERT_EQ(h.parameters_[i / 8].signature_size, signature_size);
            ASSERT_EQ(h.parameters_[i / 8].num_hashes, index_params.num_hashes);
        }
    }

    // check ratio of ones and zeros
    std::vector<std::map<std::string, size_t> > num_ones(h.parameters_.size());
    for (size_t i = 0; i < h.parameters_.size(); i++) {
        for (size_t j = 0; j < h.parameters_[i].signature_size; j++) {
            for (size_t k = 0; k < h.page_size_; k++) {
                uint8_t d = data[i][j * h.page_size_ + k];
                for (size_t o = 0; o < 8; o++) {
                    size_t file_names_index = i * h.page_size_ * 8 + k * 8 + o;
                    if (file_names_index < h.file_names_.size()) {
                        std::string file_name = h.file_names_[file_names_index];
                        num_ones[i][file_name] += (d & (1 << o)) >> o;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < h.parameters_.size(); i++) {
        double set_bit_ratio = cobs::calc_average_set_bit_ratio(
            h.parameters_[i].signature_size, 3, 0.1);
        double num_ones_average = set_bit_ratio * h.parameters_[i].signature_size;
        for (auto& no : num_ones[i]) {
            ASSERT_LE(no.second, num_ones_average * 1.02);
        }
    }

    // check content of compact index against partial classic indexes
    std::vector<std::vector<uint8_t> > indices;
    for (auto& p : fs::directory_iterator(cobs_2_dir)) {
        if (fs::is_directory(p)) {
            for (const fs::path& cobs_p : fs::directory_iterator(p)) {
                if (cobs::file_has_header<cobs::ClassicIndexHeader>(cobs_p)) {
                    cobs::ClassicIndexHeader cih;
                    std::vector<uint8_t> data;
                    cih.read_file(cobs_p, data);
                    indices.push_back(data);
                }
            }
        }
    }

    std::sort(indices.begin(), indices.end(),
              [](auto& i1, auto& i2) {
                  return i1.size() < i2.size();
              });

    ASSERT_EQ(indices.size(), data.size());
    for (size_t i = 0; i < indices.size() - 1; i++) {
        ASSERT_EQ(indices[i].size(), data[i].size());
        for (size_t j = 0; j < indices[i].size(); j++) {
            ASSERT_EQ(indices[i].data()[j], data[i][j]);
        }
    }
}

/******************************************************************************/
