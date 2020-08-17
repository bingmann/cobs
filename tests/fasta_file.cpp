/*******************************************************************************
 * tests/fasta_file.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/document_list.hpp>
#include <cobs/fasta_file.hpp>
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/query/classic_search.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path input_dir = "data/fasta/";
static fs::path base_dir = "data/fasta_index";
static fs::path index_path = base_dir / "index.cobs_classic";
static fs::path tmp_path = base_dir / "tmp";

class fasta : public ::testing::Test
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

TEST_F(fasta, process_kmers) {
    cobs::FastaFile fasta1(input_dir / "sample1.fasta");
    die_unequal(fasta1.size(), 3219u);

    cobs::FastaFile fasta7(input_dir / "sample7.fasta.gz");
    die_unequal(fasta7.size(), 1659u);

    size_t nterms = fasta7.num_terms(31);
    die_unequal(nterms, 15u * (76 - 31 + 1));

    size_t check = 0;
    fasta7.process_terms(
        31, [&](const tlx::string_view& s) {
            LOG0 << s.to_string();
            check++;
        });
    die_unequal(nterms, check);
}

TEST_F(fasta, document_list) {
    static constexpr bool debug = false;

    cobs::DocumentList doc_list(input_dir);

    die_unequal(doc_list.list().size(), 7u);

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
                ASSERT_EQ(7u, result.size());

                for (size_t i = 0; i < result.size(); ++i) {
                    sLOG << result[i].score << result[i].doc_name;

                    if (result[i].doc_name == de.name_) {
                        ASSERT_GE(result[i].score, 1u);
                    }
                }
            });
    }
}

TEST_F(fasta, listfile) {
    cobs::DocumentList doc_list("data/fasta_files.list");
    die_unequal(doc_list.list().size(), 4u);
}

/******************************************************************************/
