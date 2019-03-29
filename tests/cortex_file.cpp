/*******************************************************************************
 * tests/cortex_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/cortex_file.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path in_dir = "test/resources/cortex/";
static fs::path in_cortex = in_dir / "document.ctx";
static fs::path compare_document = in_dir / "document_sorted.txt";

TEST(cortex, process_kmers) {
    cobs::CortexFile ctx(in_cortex);

    // check contents
    die_unequal(ctx.version_, 6u);
    die_unequal(ctx.kmer_size_, 31u);
    die_unequal(ctx.num_words_per_kmer_, 1u);
    die_unequal(ctx.num_colors_, 1u);
    die_unequal(ctx.name_, "DRR030535");

    die_unequal(ctx.num_kmers(), 24158u);

    // process kmers
    std::vector<std::string> kmer_list;
    ctx.process_kmers<31>(
        [&](cobs::KMer<31>& m) {
            kmer_list.emplace_back(m.string());
        });

    die_unequal(ctx.num_kmers(), kmer_list.size());
    std::sort(kmer_list.begin(), kmer_list.end());

    // compare with ground truth
    std::ifstream ifs(compare_document);
    std::string line;
    size_t i = 0;
    while (std::getline(ifs, line)) {
        ASSERT_EQ(line, kmer_list[i]);
        i++;
    }
}

/******************************************************************************/
