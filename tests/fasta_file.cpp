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
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/classic_search.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path in_dir = "test/resources/fasta/";
static fs::path index_dir = "test/fasta_index/index";
static fs::path index_path = index_dir / "index.cobs_classic";

class fasta : public ::testing::Test
{
protected:
    void SetUp() final {
        cobs::error_code ec;
        fs::remove_all(index_dir, ec);
    }
    void TearDown() final {
        cobs::error_code ec;
        fs::remove_all(index_dir, ec);
    }
};

TEST_F(fasta, process_kmers) {
    cobs::FastaFile fasta1(in_dir / "sample1.fasta");
    die_unequal(fasta1.size(), 3219u);

    cobs::FastaFile fasta7(in_dir / "sample7.fasta");
    die_unequal(fasta7.size(), 1659u);

    size_t nterms = fasta7.num_terms(31);
    die_unequal(nterms, 15u * (77 - 31 + 1));

    size_t check = 0;
    fasta7.process_terms(31, [&](const cobs::string_view&) { check++; });
    die_unequal(nterms, check);
}

TEST_F(fasta, document_list) {
    static constexpr bool debug = false;

    cobs::DocumentList doc_list(in_dir);

    die_unequal(doc_list.list().size(), 7u);

    // construct classic index
    cobs::classic_index::IndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.batch_bytes = 0;

    cobs::classic_index::construct(in_dir, index_dir, index_params);
    cobs::query::classic_index::mmap s_mmap(index_path);
    cobs::query::ClassicSearch s_base(s_mmap);

    // run queries for each kmer in the documents
    for (const cobs::DocumentEntry& de : doc_list.list()) {
        LOG << de.name_;
        de.process_terms(
            /* term_size */ 31,
            [&](const cobs::string_view& term) {
                std::string query(term);

                std::vector<std::pair<uint16_t, std::string> > result;
                s_base.search(query, result);
                ASSERT_EQ(7u, result.size());

                for (size_t i = 0; i < result.size(); ++i) {
                    sLOG << result[i].first << result[i].second;

                    if (result[i].second == de.name_) {
                        ASSERT_GE(result[i].first, 1u);
                    }
                }
            });
    }
}

/******************************************************************************/
