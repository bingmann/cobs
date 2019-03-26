/*******************************************************************************
 * tests/fasta_file.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/fasta_file.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path in_dir = "test/resources/fasta/";

TEST(fasta, process_kmers1) {
    cobs::FastaFile fasta(in_dir / "sample1.fasta");

    die_unequal(fasta.num_documents(), 1u);
}

TEST(fasta, process_kmers2) {
    cobs::FastaFile fasta(in_dir / "sample2.fasta");

    die_unequal(fasta.num_documents(), 5u);

    die_unequal(fasta.index_[0].size, 256u);
    die_unequal(fasta.index_[4].size, 438u);

    die_unequal(fasta.num_terms(0, 31), 256u - 31u + 1u);

    size_t count = 0;
    fasta.process_terms(0, 31, [&](const std::string&) { ++count; });
    die_unequal(fasta.num_terms(0, 31), count);
}

/******************************************************************************/
