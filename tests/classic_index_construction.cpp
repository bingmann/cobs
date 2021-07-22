/*******************************************************************************
 * tests/classic_index_construction.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/
#include <fstream>
#include <algorithm>

#include <fstream>
#include <algorithm>

#include "test_util.hpp"
#include <cobs/file/classic_index_header.hpp>
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

// Compare two classic indices. Return true if the bloom filters of both files are the same.
bool compare_classic_indices(const std::string& filename1, const std::string& filename2)
{
    std::ifstream ifs1, ifs2;

    cobs::deserialize_header<cobs::ClassicIndexHeader>(ifs1, filename1);
    cobs::deserialize_header<cobs::ClassicIndexHeader>(ifs2, filename2);

    std::istreambuf_iterator<char> begin1(ifs1);
    std::istreambuf_iterator<char> begin2(ifs2);
  
    return std::equal(begin1,std::istreambuf_iterator<char>(),begin2); //Second argument is end-of-range iterator
}

// Compare two files. Return true if the contents of both files are the same.
bool compare_files(const std::string& filename1, const std::string& filename2)
{
    std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
    std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end
    const std::ifstream::pos_type fileSize = file1.tellg();

    if (fileSize != file2.tellg()) {
        return false; //different file size
    }

    file1.seekg(0); //rewind
    file2.seekg(0); //rewind

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    return std::equal(begin1,std::istreambuf_iterator<char>(),begin2); //Second argument is end-of-range iterator
}

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
    ASSERT_EQ(h.file_names_.size(), 33u);
    ASSERT_EQ(h.num_hashes_, 3u);
    ASSERT_EQ(h.file_names_.size(), paths.size());
    for (size_t i = 0; i < h.file_names_.size(); i++) {
        ASSERT_EQ(h.file_names_[i], cobs::base_name(paths[i]));
    }

    // check ratio of zeros/ones
    std::map<std::string, size_t> num_ones;
    for (size_t j = 0; j < h.signature_size_; j++) {
        for (size_t k = 0; k < h.row_size(); k++) {
            uint8_t d = data[j * h.row_size() + k];
            for (size_t o = 0; o < 8; o++) {
                size_t file_names_index = k * 8 + o;
                if (file_names_index < h.file_names_.size()) {
                    std::string file_name = h.file_names_[file_names_index];
                    num_ones[file_name] += (d & (1 << o)) >> o;
                }
            }
        }
    }

    double set_bit_ratio =
        cobs::calc_average_set_bit_ratio(h.signature_size_, 3, 0.1);
    double num_ones_average = set_bit_ratio * h.signature_size_;
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

    std::vector<cobs::SearchResult> result;

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
                    if (r.doc_name == doc_match && r.score > 0)
                        found = true;
                }
                ASSERT_TRUE(found);
            }
        }
    }
}

TEST_F(classic_index_construction, combined_index_same_as_classic_constructed) {
    // This test starts with 2 copies of the same randomly generated document.
    // We build a classic index for each of these two documents.
    // We then combine these two classic indices into one combined index.
    // The combined index should be the same as the classic index generated
    // through `cobs:classic_construct` on these two documents.
    using cobs::pad_index;
    fs::create_directories(index_dir);
    fs::create_directories(index_dir/pad_index(0));
    fs::create_directories(index_dir/pad_index(1));
    fs::create_directories(index_dir/pad_index(2));

    // prepare 2 copy of a randomly generated document
    std::string random_doc_src_string = cobs::random_sequence(1000, 1);
    auto random_docs = generate_documents_one(random_doc_src_string, 1);
    generate_test_case(random_docs, "random_", input_dir/pad_index(0));
    generate_test_case(random_docs, "random_", input_dir/pad_index(1));

    cobs::ClassicIndexParameters index_params;
    index_params.false_positive_rate = 0.001; // in order to use large signature size
    index_params.mem_bytes = 80;
    index_params.num_threads = 1;
    index_params.continue_ = true;

    // generate a classic index for each document
    cobs::classic_construct(cobs::DocumentList(input_dir/pad_index(0)),
                           index_dir/pad_index(0)/(pad_index(0) + ".cobs_classic"),
                           tmp_path, index_params);
    cobs::classic_construct(cobs::DocumentList(input_dir/pad_index(1)),
                           index_dir/pad_index(0)/(pad_index(1) + ".cobs_classic"),
                           tmp_path, index_params);

    // generate a combined index fro both classic constructed index
    fs::path combined_index;
    cobs::classic_combine(index_dir/pad_index(0), index_dir/pad_index(1), combined_index,
                          80, 1, false);

    // generate a classic index for both docs through classic_construct
    std::string classic_constructed_index = index_dir/pad_index(2)/(pad_index(0) +
            ".cobs_classic");
    cobs::classic_construct(cobs::DocumentList(input_dir), classic_constructed_index,
            tmp_path, index_params);

    ASSERT_TRUE(compare_files(combined_index.string(), classic_constructed_index));
}

TEST_F(classic_index_construction, same_documents_combined_into_same_index) {
    // This test starts with 18 copies of the same randomly generated document.
    // These documents are split in 4 groups: g1 with 1 copy, g2 with 2 copies,
    // g7 with 7 copies and g8 with 8 copies.
    // A classic index is constructed through `cobs::classic_construct` for these
    // 4 groups: c1, c2, c7 and c8.
    // We then combine these 4 classic indices in this way: c1 + c8 = c1c8 and
    // c2 + c7 = c2c7.
    // The point of this test is to verify that c1c8 and c2c7 are the same.
    using cobs::pad_index;
    fs::create_directories(index_dir);
    fs::create_directories(index_dir/pad_index(0));
    fs::create_directories(index_dir/pad_index(1));
    fs::create_directories(index_dir/pad_index(18));
    fs::create_directories(index_dir/pad_index(27));

    // prepare 4 groups of copies of a randomly generated document
    std::string random_doc_src_string = cobs::random_sequence(1000, 1);
    auto random_doc_one_copy = generate_documents_one(random_doc_src_string, 1);
    auto random_doc_two_copies = std::vector<cobs::KMerBuffer<31> >(2, random_doc_one_copy[0]);
    auto random_doc_seven_copies = std::vector<cobs::KMerBuffer<31> >(7, random_doc_one_copy[0]);
    auto random_doc_eight_copies = std::vector<cobs::KMerBuffer<31> >(8, random_doc_one_copy[0]);
    generate_test_case(random_doc_one_copy, "random_", input_dir/pad_index(1));
    generate_test_case(random_doc_two_copies, "random_", input_dir/pad_index(2));
    generate_test_case(random_doc_seven_copies, "random_", input_dir/pad_index(7));
    generate_test_case(random_doc_eight_copies, "random_", input_dir/pad_index(8));

    cobs::ClassicIndexParameters index_params;
    index_params.false_positive_rate = 0.001; // in order to use large signature size
    index_params.mem_bytes = 80;
    index_params.num_threads = 1;
    index_params.continue_ = true;

    // generate a classic index for each document groups
    cobs::classic_construct(cobs::DocumentList(input_dir/pad_index(1)),
            index_dir/pad_index(18)/(pad_index(1) + ".cobs_classic"),
            tmp_path, index_params);
    cobs::classic_construct(cobs::DocumentList(input_dir/pad_index(8)),
            index_dir/pad_index(18)/(pad_index(8) + ".cobs_classic"),
            tmp_path, index_params);
    cobs::classic_construct(cobs::DocumentList(input_dir/pad_index(2)),
            index_dir/pad_index(27)/(pad_index(2) + ".cobs_classic"),
            tmp_path, index_params);
    cobs::classic_construct(cobs::DocumentList(input_dir/pad_index(7)),
            index_dir/pad_index(27)/(pad_index(7) + ".cobs_classic"),
            tmp_path, index_params);

    // generate a combined index fro both classic constructed index
    fs::path c1c8, c2c7;
    cobs::classic_combine(index_dir/pad_index(18), index_dir/pad_index(0), c1c8,
                          80, 1, false);
    cobs::classic_combine(index_dir/pad_index(27), index_dir/pad_index(1), c2c7,
                          80, 1, false);

    ASSERT_TRUE(compare_classic_indices(c1c8.string(), c2c7.string()));
}

/******************************************************************************/
