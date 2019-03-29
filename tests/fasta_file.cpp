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

TEST_F(fasta, process_kmers1) {
    cobs::FastaFile fasta(in_dir / "sample1.fasta");

    die_unequal(fasta.num_documents(), 1u);
}

TEST_F(fasta, process_kmers2) {
    cobs::FastaFile fasta(in_dir / "sample2.fasta");

    die_unequal(fasta.num_documents(), 5u);

    die_unequal(fasta.size(0), 256u);
    die_unequal(fasta.size(4), 438u);

    size_t count = 0;
    fasta.process_terms(0, 31, [&](const cobs::string_view&) { ++count; });
    die_unequal(fasta.size(0) - 30, count);
}

TEST_F(fasta, DISABLED_document_list) {
    static constexpr bool debug = false;

    cobs::DocumentList doc_list(in_dir);

    die_unequal(doc_list.list().size(), 6u);

    // construct classic index
    cobs::classic_index::construct(in_dir, index_dir, 8, 1, 0.3);
    cobs::query::classic_index::mmap s_mmap(index_path);

    // run queries for each kmer in the documents
    for (const cobs::DocumentEntry& de : doc_list.list()) {
        LOG << de.name_;
        de.process_terms(
            [&](const cobs::string_view& term) {
                std::string query(term);

                std::vector<std::pair<uint16_t, std::string> > result;
                s_mmap.search(query, 31, result);
                ASSERT_EQ(6u, result.size());

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
