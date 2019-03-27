/*******************************************************************************
 * tests/fasta_file.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/document_list.hpp>
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

    die_unequal(fasta.size(0), 256u);
    die_unequal(fasta.size(4), 438u);

    size_t count = 0;
    fasta.process_terms(0, 31, [&](const std::string&) { ++count; });
    die_unequal(fasta.size(0) - 30, count);
}

TEST(fasta, document_list) {
    cobs::DocumentList doc_list(in_dir);

    die_unequal(doc_list.list().size(), 6u);
}

/******************************************************************************/
