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
#include <cobs/query/classic_index/mmap_search_file.hpp>
#include <cobs/query/classic_search.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path input_dir = "test/resources/fastq/";
static fs::path base_dir = "test/fastq_index";
static fs::path index_path = base_dir / "index.cobs_classic";
static fs::path tmp_path = base_dir / "tmp";

class fastq : public ::testing::Test
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

TEST_F(fastq, process_kmers) {
    cobs::FastqFile fastq1(input_dir / "sample1.fastq");
    die_unequal(fastq1.size(), 3518u);

    cobs::FastqFile fastq2(input_dir / "sample2.fastq.gz");
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

    cobs::DocumentList doc_list(input_dir);

    die_unequal(doc_list.list().size(), 3u);

    // construct classic index
    cobs::ClassicIndexParameters index_params;
    index_params.num_hashes = 3;
    index_params.false_positive_rate = 0.1;

    cobs::classic_construct(input_dir, index_path, tmp_path, index_params);
    cobs::ClassicIndexMMapSearchFile s_mmap(index_path);
    cobs::ClassicSearch s_base(s_mmap);

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
