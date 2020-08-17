/*******************************************************************************
 * tests/fasta_multifile.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/document_list.hpp>
#include <cobs/fasta_multifile.hpp>
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/query/classic_search.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path input_dir = "data/fasta_multi/";
static fs::path base_dir = "data/fasta_multi_index";
static fs::path index_path = base_dir / "index.cobs_classic";
static fs::path tmp_path = base_dir / "tmp";

class fasta_multi : public ::testing::Test
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

TEST_F(fasta_multi, process_kmers1) {
    cobs::FastaMultifile fasta_multi(input_dir / "sample1.mfasta");

    die_unequal(fasta_multi.num_documents(), 1u);
}

TEST_F(fasta_multi, process_kmers2) {
    cobs::FastaMultifile fasta_multi(input_dir / "sample2.mfasta");

    die_unequal(fasta_multi.num_documents(), 5u);

    die_unequal(fasta_multi.size(0), 256u);
    die_unequal(fasta_multi.size(4), 438u);

    size_t count = 0;
    fasta_multi.process_terms(0, 31, [&](const tlx::string_view&) { ++count; });
    die_unequal(fasta_multi.size(0) - 30, count);
}

TEST_F(fasta_multi, document_list) {
    static constexpr bool debug = false;

    cobs::DocumentList doc_list(input_dir);

    die_unequal(doc_list.list().size(), 6u);

    // construct classic index
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.canonicalize = 0;

    cobs::classic_construct(
        cobs::DocumentList(input_dir), index_path, tmp_path, index_params);
    cobs::ClassicSearch s_base(
        std::make_shared<cobs::ClassicIndexMMapSearchFile>(index_path));

    // run queries for each kmer in the documents
    for (const cobs::DocumentEntry& de : doc_list.list()) {
        LOG << de.name_;
        de.process_terms(
            /* term_size */ 31,
            [&](const tlx::string_view& term) {
                std::string query = term.to_string();

                std::vector<cobs::SearchResult> result;
                s_base.search(query, result);
                ASSERT_EQ(6u, result.size());

                for (size_t i = 0; i < result.size(); ++i) {
                    sLOG << result[i].score << result[i].doc_name;

                    if (result[i].doc_name == de.name_) {
                        ASSERT_GE(result[i].score, 1u);
                    }
                }
            });
    }
}

/******************************************************************************/
