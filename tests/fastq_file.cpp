/*******************************************************************************
 * tests/fastq_file.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/document_list.hpp>
#include <cobs/fastq_file.hpp>
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/classic_search.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path in_dir = "test/resources/fastq/";
static fs::path index_dir = "test/fastq_index/index";
static fs::path index_path = index_dir / "index.cobs_classic";

class fastq : public ::testing::Test
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

TEST_F(fastq, process_kmers) {
    cobs::FastqFile fastq1(in_dir / "sample1.fastq");
    die_unequal(fastq1.size(), 3518u);

    cobs::FastqFile fastq2(in_dir / "sample2.fastq.gz");
    die_unequal(fastq2.size(), 3001u);

    size_t nterms = fastq2.num_terms(31);
    // die_unequal(nterms, 17u * (65 - 31 + 1));

    size_t check = 0;
    fastq2.process_terms(
        31, [&](const cobs::string_view& s) {
            LOG0 << s.to_string();
            check++;
        });
    die_unequal(nterms, check);
}

TEST_F(fastq, document_list) {
    static constexpr bool debug = false;

    cobs::DocumentList doc_list(in_dir);

    die_unequal(doc_list.list().size(), 3u);

    // construct classic index
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;
    index_params.batch_size = 16;

    cobs::classic_construct(in_dir, index_dir, index_params);
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
                ASSERT_EQ(3u, result.size());

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
