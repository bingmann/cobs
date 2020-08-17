/*******************************************************************************
 * tests/cortex_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2020 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/cortex_file.hpp>
#include <gtest/gtest.h>

#include <tlx/logger.hpp>

namespace fs = cobs::fs;

static fs::path in_dir = "data/cortex/";
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
    ctx.process_terms(
        31,
        [&](const tlx::string_view& v) {
            kmer_list.emplace_back(v.to_string());
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

TEST(cortex, sample1) {
    std::string line;

    cobs::CortexFile ctx31(in_dir / "sample1-k31.ctx");
    std::ifstream txt31(in_dir / "sample1-k31.txt");
    ctx31.process_terms(
        31,
        [&](const tlx::string_view& v) {
            ASSERT_TRUE(std::getline(txt31, line));
            ASSERT_EQ(line, v.to_string());
        });
    ASSERT_FALSE(std::getline(txt31, line));

    cobs::CortexFile ctx19(in_dir / "sample1-k19.ctx");
    std::ifstream txt19(in_dir / "sample1-k19.txt");
    ctx19.process_terms(
        19,
        [&](const tlx::string_view& v) {
            ASSERT_TRUE(std::getline(txt19, line));
            ASSERT_EQ(line, v.to_string());
        });
    ASSERT_FALSE(std::getline(txt31, line));

    cobs::CortexFile ctx15(in_dir / "sample1-k15.ctx");
    std::ifstream txt15(in_dir / "sample1-k15.txt");
    ctx15.process_terms(
        15,
        [&](const tlx::string_view& v) {
            ASSERT_TRUE(std::getline(txt15, line));
            ASSERT_EQ(line, v.to_string());
        });
    ASSERT_FALSE(std::getline(txt31, line));
}

/******************************************************************************/
