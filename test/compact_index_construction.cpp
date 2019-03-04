/*******************************************************************************
 * test/compact_index_construction.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path in_dir("test/out/compact_index_construction/input");
static fs::path documents_dir(in_dir.string() + "/documents");
static fs::path isi_2_dir(in_dir.string() + "/isi_2");
static fs::path compact_index_path(in_dir.string() + "/index.com_idx.isi");
static fs::path tmp_dir("test/out/compact_index_construction/tmp");

static std::string query = cobs::random_sequence(10000, 1);

class compact_index_construction : public ::testing::Test
{
protected:
    virtual void SetUp() {
        cobs::error_code ec;
        fs::remove_all(in_dir, ec);
        fs::remove_all(tmp_dir, ec);
        fs::create_directories(in_dir);
        fs::create_directories(tmp_dir);
    }
};

TEST_F(compact_index_construction, padding) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    size_t page_size = cobs::get_page_size();
    cobs::compact_index::create_folders(tmp_dir, in_dir, page_size);
    cobs::compact_index::construct_from_folders(in_dir, 8, 3, 0.1, page_size);
    std::ifstream ifs;
    cobs::file::deserialize_header<cobs::file::compact_index_header>(ifs, compact_index_path);
    cobs::stream_metadata smd = cobs::get_stream_metadata(ifs);
    ASSERT_EQ(smd.curr_pos % page_size, 0U);
}

TEST_F(compact_index_construction, deserialization) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());

    cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
    cobs::compact_index::construct_from_folders(in_dir, 8, 3, 0.1, 2);
    std::vector<std::vector<uint8_t> > data;
    cobs::file::compact_index_header h;
    h.read_file(compact_index_path, data);
    ASSERT_EQ(h.file_names().size(), 33U);
    ASSERT_EQ(h.parameters().size(), 3U);
    ASSERT_EQ(data.size(), 3U);
}

TEST_F(compact_index_construction, file_names) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());

    std::vector<fs::path> paths;
    fs::recursive_directory_iterator it(tmp_dir), end;
    std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
                     return cobs::file::file_is<cobs::file::document_header>(p);
                 });
    std::sort(paths.begin(), paths.end(), [](const auto& p1, const auto& p2) {
                  return fs::file_size(p1) < fs::file_size(p2);
              });
    for (size_t i = 0; i < documents.size(); i += 2 * 8) {
        size_t middle_index = std::min(i + 16, paths.size());
        std::sort(paths.begin() + i, paths.begin() + middle_index);
    }

    cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
    cobs::compact_index::construct_from_folders(in_dir, 8, 3, 0.1, 2);
    std::vector<std::vector<uint8_t> > data;
    auto h = cobs::file::deserialize_header<cobs::file::compact_index_header>(compact_index_path);
    h.read_file(compact_index_path, data);
    for (size_t i = 0; i < h.file_names().size(); i++) {
        ASSERT_EQ(h.file_names()[i], cobs::file::file_name(paths[i]));
    }
}

TEST_F(compact_index_construction, parameters) {
    uint64_t num_hashes = 3;
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
    cobs::compact_index::construct_from_folders(in_dir, 8, num_hashes, 0.1, 2);
    std::vector<std::vector<uint8_t> > data;
    auto h = cobs::file::deserialize_header<cobs::file::compact_index_header>(compact_index_path);

    std::vector<uint64_t> document_sizes;
    std::vector<cobs::file::compact_index_header::parameter> parameters;
    for (const fs::path& p : fs::recursive_directory_iterator(documents_dir)) {
        if (cobs::file::file_is<cobs::file::classic_index_header>(p)) {
            document_sizes.push_back(fs::file_size(p));
        }
    }

    std::sort(document_sizes.begin(), document_sizes.end());
    for (size_t i = 0; i < document_sizes.size(); i++) {
        if (i % 8 == 7) {
            uint64_t signature_size = cobs::calc_signature_size(
                document_sizes[i] / 8, num_hashes, 0.1);
            ASSERT_EQ(h.parameters()[i / 8].signature_size, signature_size);
            ASSERT_EQ(h.parameters()[i / 8].num_hashes, num_hashes);
        }
    }
}

TEST_F(compact_index_construction, num_kmers_calculation) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    fs::path path_document(tmp_dir.string() + "/document_00.doc.isi");
    cobs::Document<31> doc;
    cobs::file::document_header hdoc;
    doc.deserialize(path_document, hdoc);

    size_t file_size = fs::file_size(path_document);
    ASSERT_EQ(doc.data().size(), file_size / 8 - 4);
}

TEST_F(compact_index_construction, num_ones) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
    cobs::compact_index::construct_from_folders(in_dir, 8, 3, 0.1, 2);
    std::vector<std::vector<uint8_t> > data;
    cobs::file::compact_index_header h;
    h.read_file(compact_index_path, data);

    std::vector<std::map<std::string, size_t> > num_ones(h.parameters().size());
    for (size_t i = 0; i < h.parameters().size(); i++) {
        for (size_t j = 0; j < h.parameters()[i].signature_size; j++) {
            for (size_t k = 0; k < h.page_size(); k++) {
                uint8_t d = data[i][j * h.page_size() + k];
                for (size_t o = 0; o < 8; o++) {
                    size_t file_names_index = i * h.page_size() * 8 + k * 8 + o;
                    if (file_names_index < h.file_names().size()) {
                        std::string file_name = h.file_names()[file_names_index];
                        num_ones[i][file_name] += (d & (1 << o)) >> o;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < h.parameters().size(); i++) {
        double set_bit_ratio = cobs::calc_average_set_bit_ratio(
            h.parameters()[i].signature_size, 3, 0.1);
        double num_ones_average = set_bit_ratio * h.parameters()[i].signature_size;
        for (auto& no : num_ones[i]) {
            ASSERT_LE(no.second, num_ones_average * 1.02);
        }
    }
}

TEST_F(compact_index_construction, content) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::compact_index::create_folders(tmp_dir, in_dir, 2);
    cobs::compact_index::construct_from_folders(in_dir, 8, 3, 0.1, 2);
    std::vector<std::vector<uint8_t> > cisi_data;
    cobs::file::compact_index_header h;
    h.read_file(compact_index_path, cisi_data);

    std::vector<std::vector<uint8_t> > indices;
    for (auto& p : fs::directory_iterator(isi_2_dir)) {
        if (fs::is_directory(p)) {
            for (const fs::path& isi_p : fs::directory_iterator(p)) {
                if (cobs::file::file_is<cobs::file::classic_index_header>(isi_p)) {
                    cobs::file::classic_index_header cih;
                    std::vector<uint8_t> data;
                    cih.read_file(isi_p, data);
                    indices.push_back(data);
                }
            }
        }
    }

    std::sort(indices.begin(), indices.end(), [](auto& i1, auto& i2) {
                  return i1.size() < i2.size();
              });

    ASSERT_EQ(indices.size(), cisi_data.size());
    for (size_t i = 0; i < indices.size() - 1; i++) {
        ASSERT_EQ(indices[i].size(), cisi_data[i].size());
        for (size_t j = 0; j < indices[i].size(); j++) {
            ASSERT_EQ(indices[i].data()[j], cisi_data[i][j]);
        }
    }
}

/******************************************************************************/
