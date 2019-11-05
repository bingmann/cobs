/*******************************************************************************
 * tests/classic_index_construction.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path base_dir = "data/classic_index_construction";
static fs::path input_dir = base_dir / "input";
static fs::path index_dir = base_dir / "index";
static fs::path index_file = base_dir / "index.cobs_classic";
static fs::path tmp_path = base_dir / "tmp";

class classic_index_construction : public ::testing::Test
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

    cobs::classic_construct(
        cobs::DocumentList(input_dir), index_file, tmp_path, index_params);

    // read classic index and check header fields
    std::vector<uint8_t> data;
    cobs::ClassicIndexHeader h;
    h.read_file(index_file, data);
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

TEST_F(classic_index_construction, combine) {
    using cobs::pad_index;
    fs::create_directories(index_dir);
    // generate 10 individual sets of documents and construct indices
    using DocumentSet = std::vector<cobs::KMerBuffer<31> >;
    std::vector<DocumentSet> doc_sets;
    for (size_t i = 0; i < 10; ++i) {
        std::string query = cobs::random_sequence(10000, /* seed */ i + 1);
        auto documents = generate_documents_all(
            query, /* num_documents */ 3, /* num_terms */ 100);
        generate_test_case(
            documents, /* prefix */ "set_" + pad_index(i) + "_",
            input_dir / pad_index(i));
        doc_sets.emplace_back(std::move(documents));

        // construct classic index
        cobs::ClassicIndexParameters index_params;
        index_params.num_hashes = 3;
        index_params.false_positive_rate = 0.1;

        cobs::classic_construct(
            cobs::DocumentList(input_dir / pad_index(i)),
            index_dir / (pad_index(i) + ".cobs_classic"),
            tmp_path, index_params);
    }

    fs::path result_file;
    cobs::classic_combine(
        index_dir, index_file, result_file,
        /* mem_bytes */ 128 * 1024 * 1024, /* num_threads */ 4,
        /* keep_temporary */ false);

    // check result by querying for document terms
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(result_file));

    std::vector<std::pair<uint16_t, std::string> > result;

    for (size_t ds = 0; ds < 10; ++ds) {
        for (size_t d = 0; d < doc_sets[ds].size(); ++d) {
            for (size_t i = 0; i < doc_sets[ds][d].num_kmers(); ++i) {
                std::string doc_match =
                    "set_" + pad_index(ds) + "_document_" + pad_index(d);
                std::string kmer = doc_sets[ds][d][i].string();
                LOG0 << kmer;

                s_base.search(kmer, result);

                bool found = false;
                for (auto& r : result) {
                    if (r.second == doc_match && r.first > 0)
                        found = true;
                }
                ASSERT_TRUE(found);
            }
        }
    }
}

/******************************************************************************/
