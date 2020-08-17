/*******************************************************************************
 * tests/text_file.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/document_list.hpp>
#include <cobs/text_file.hpp>
#include <gtest/gtest.h>

namespace fs = cobs::fs;

static fs::path in_dir = "data/text/";

TEST(text, process_txt1) {
    cobs::TextFile text(in_dir / "sample1.txt");

    die_unequal(text.size(), 76u);

    size_t count = 0;
    text.process_terms(31, [&](const tlx::string_view&) { ++count; });
    die_unequal(text.size() - 30, count);
}

TEST(text, document_list) {
    cobs::DocumentList doc_list(in_dir);

    die_unequal(doc_list.list().size(), 2u);
}

/******************************************************************************/
