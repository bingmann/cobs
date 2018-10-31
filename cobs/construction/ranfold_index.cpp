/*******************************************************************************
 * cobs/construction/ranfold_index.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <random>
#include <set>

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/ranfold_index.hpp>
#include <cobs/document.hpp>
#include <cobs/file/ranfold_index_header.hpp>
#include <cobs/kmer.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>
#include <tlx/math/popcount.hpp>

namespace cobs::ranfold_index {

void set_bit(std::vector<uint8_t>& data,
             const file::ranfold_index_header& rih,
             uint64_t term_index, uint64_t document_index) {
    size_t x = rih.m_doc_space_bytes * term_index + document_index / 8;
    die_unless(x < data.size());
    data[x] |= 1 << (document_index % 8);
}

template <typename RandomGenerator>
document<31> document_generator(size_t document_size, RandomGenerator& rng) {
    document<31> doc;

    // create random document
    doc.data().clear();
    for (size_t j = 0; j < document_size; ++j) {
        kmer<31> m;
        // should canonicalize the kmer, but since we can assume uniform
        // hashing, this is not needed.
        m.fill_random(rng);
        doc.data().push_back(m);
    }

    return doc;
}

void mark_document(const document<31>& doc,
                   const file::ranfold_index_header& rih,
                   std::vector<uint8_t>& data,
                   size_t document_index) {
    for (size_t j = 0; j < doc.data().size(); ++j) {
        // process term hashes
        classic_index::process_hashes(
            doc.data().data() + j, 8,
            rih.m_term_space, rih.m_term_hashes,
            [&](uint64_t hash) {
                set_bit(data, rih, hash, document_index);
            });
    }
}

void construct_random(const fs::path& out_file,
                      uint64_t term_space, uint64_t num_term_hashes,
                      uint64_t doc_space_bits, uint64_t num_doc_hashes,
                      uint64_t num_documents, size_t document_size,
                      size_t seed) {
    timer t;

    file::ranfold_index_header rih;
    rih.m_file_names.reserve(num_documents);
    for (size_t i = 0; i < num_documents; ++i) {
        rih.m_file_names.push_back("file_" + std::to_string(i));
    }

    // compress factor 2
    doc_space_bits = num_documents / 2;

    rih.m_term_space = term_space;
    rih.m_term_hashes = num_term_hashes;
    rih.m_doc_space_bytes = (doc_space_bits + 7) / 8;
    rih.m_doc_hashes = num_doc_hashes;

    doc_space_bits = rih.m_doc_space_bytes * 8;

    std::vector<uint8_t> data;
    data.resize(rih.m_doc_space_bytes * rih.m_term_space);

    std::mt19937 rng(seed);

    t.active("generate");

    std::vector<document<31> > docset;

    for (size_t doc_id = 0; doc_id < num_documents / 2; ++doc_id) {

        while (docset.size() < 2) {
            docset.push_back(document_generator(document_size, rng));
        }

        // put two documents into one index
        mark_document(docset[0], rih, data, doc_id / 2);
        mark_document(docset[1], rih, data, doc_id / 2);

        docset.clear();
    }

    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "ratio of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    rih.write_file(out_file, data);
    t.stop();

    std::cout << t;
}

} // namespace cobs::ranfold_index

/******************************************************************************/
