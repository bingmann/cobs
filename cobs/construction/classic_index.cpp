/*******************************************************************************
 * cobs/construction/classic_index.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <random>

#include <cobs/construction/classic_index.hpp>
#include <cobs/document_list.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/kmer.hpp>
#include <cobs/text_file.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>
#include <tlx/math/popcount.hpp>

namespace cobs::classic_index {

void set_bit(std::vector<uint8_t>& data,
             const ClassicIndexHeader& cih,
             uint64_t pos, uint64_t doc_index) {
    data[cih.block_size() * pos + doc_index / 8] |= 1 << (doc_index % 8);
}

void process_term(const std::string& term,
                  std::vector<uint8_t>& data,
                  const ClassicIndexHeader& cih,
                  size_t doc_index) {
    process_hashes(term.data(), term.size(),
                   cih.signature_size(), cih.num_hashes(),
                   [&](uint64_t hash) {
                       set_bit(data, cih, hash, doc_index);
                   });
}

template <typename Callback>
void process_document(const DocumentEntry& doc_entry,
                      Callback callback) {
    if (doc_entry.type_ == FileType::Text) {
        TextFile text(doc_entry.path_);
        text.process_terms(31, callback);
    }
    else if (doc_entry.type_ == FileType::Cortex) {
        CortexFile ctx(doc_entry.path_);
        ctx.process_terms<31>(callback);
    }
    else if (doc_entry.type_ == FileType::KMerBuffer) {
        KMerBuffer<31> doc;
        KMerBufferHeader dh;
        doc.deserialize(doc_entry.path_, dh);

        std::string term;
        term.reserve(32);

        for (uint64_t j = 0; j < doc.data().size(); j++) {
            doc.data()[j].canonicalize();
            doc.data()[j].to_string(&term);
            callback(term);
        }
    }
    else if (doc_entry.type_ == FileType::Fasta) {
        FastaFile fasta(doc_entry.path_);
        fasta.process_terms(doc_entry.subdoc_index_, 31, callback);
    }
}

void process_batch(const std::vector<DocumentEntry>& paths,
                   const fs::path& out_file, std::vector<uint8_t>& data,
                   ClassicIndexHeader& cih, Timer& t) {

    for (uint64_t i = 0; i < paths.size(); i++) {
        t.active("read");
        cih.file_names()[i] = paths[i].name_;

        // TODO: parallel for over setting of bits of the SAME document
        process_document(paths[i],
                         [&](const std::string& term) {
                             process_term(term, data, cih, i);
                         });
    }
    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "ratio of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    cih.write_file(out_file, data);
    t.stop();
}

void combine(std::vector<std::pair<std::ifstream, uint64_t> >& ifstreams,
             const fs::path& out_file,
             uint64_t signature_size, uint64_t block_size, uint64_t num_hash,
             Timer& t, const std::vector<std::string>& file_names) {
    std::ofstream ofs;
    ClassicIndexHeader cih(signature_size, num_hash, file_names);
    serialize_header(ofs, out_file, cih);

    std::vector<char> block(block_size);
    for (uint64_t i = 0; i < signature_size; i++) {
        uint64_t pos = 0;
        t.active("read");
        for (auto& ifs : ifstreams) {
            ifs.first.read(block.data() + pos, ifs.second);
            pos += ifs.second;
        }
        t.active("write");
        ofs.write(block.data(), block_size);
    }
    t.stop();
}

void construct_from_documents(DocumentList& doc_list, const fs::path& out_dir,
                              uint64_t signature_size, uint64_t num_hashes,
                              uint64_t batch_size) {
    Timer t;
    ClassicIndexHeader cih(signature_size, num_hashes);
    fs::create_directories(out_dir);

    std::vector<uint8_t> data;
    doc_list.process_batches(
        batch_size,
        [&](const std::vector<DocumentEntry>& paths, std::string out_file) {
            fs::path out_path =
                out_dir / (out_file + ClassicIndexHeader::file_extension);
            cih.file_names().resize(paths.size());
            data.resize(signature_size * cih.block_size());
            std::fill(data.begin(), data.end(), 0);
            process_batch(paths, out_path, data, cih, t);
        });
    std::cout << t;
}

bool combine(const fs::path& in_dir, const fs::path& out_dir, uint64_t batch_size) {
    Timer t;
    std::vector<std::pair<std::ifstream, uint64_t> > ifstreams;
    std::vector<std::string> file_names;
    uint64_t signature_size = 0;
    uint64_t num_hashes = 0;
    bool all_combined =
        process_file_batches(
            in_dir, out_dir, batch_size, ClassicIndexHeader::file_extension,
            [](const fs::path& path) {
                return file_has_header<ClassicIndexHeader>(path);
            },
            [&](const std::vector<fs::path>& paths, const fs::path& out_file) {
                uint64_t new_block_size = 0;
                for (size_t i = 0; i < paths.size(); i++) {
                    ifstreams.emplace_back(std::make_pair(std::ifstream(), 0));
                    auto cih = deserialize_header<ClassicIndexHeader>(ifstreams.back().first, paths[i]);
                    if (signature_size == 0) {
                        signature_size = cih.signature_size();
                        num_hashes = cih.num_hashes();
                    }
                    die_unless(cih.signature_size() == signature_size);
                    die_unless(cih.num_hashes() == num_hashes);
#ifndef isi_test
                    if (i < paths.size() - 2) {
                        // todo doesnt work because of padding for compact, which means there could be two files with less file_names
                        // todo quickfix with -2 to allow for paddding
                        // die_unless(cih.file_names().size() == 8 * cih.block_size());
                    }
#endif
                    ifstreams.back().second = cih.block_size();
                    new_block_size += cih.block_size();
                    std::copy(cih.file_names().begin(), cih.file_names().end(), std::back_inserter(file_names));
                }
                combine(ifstreams, out_file, signature_size, new_block_size, num_hashes, t, file_names);
                ifstreams.clear();
                file_names.clear();
            });
    std::cout << t;
    return all_combined;
}

uint64_t get_max_file_size(DocumentList& doc_list) {
    // sort document by file size (as approximation to the number of kmers)
    std::vector<DocumentEntry> paths = doc_list.list();
    std::sort(paths.begin(), paths.end(),
              [](const DocumentEntry& p1, const DocumentEntry& p2) {
                  return (std::tie(p1.size_, p1.path_) >
                          std::tie(p2.size_, p2.path_));
              });

    // look into largest file and return number of elements
    if (paths[0].path_.extension() == ".ctx") {
        CortexFile ctx(paths[0].path_.string());
        size_t max_num_elements = ctx.num_kmers();
        sLOG1 << "CTX: max_num_elements" << max_num_elements;
        return max_num_elements;
    }
    else if (paths[0].path_.extension() == ".cobs_doc") {
        KMerBufferHeader dh;
        KMerBuffer<31> doc;
        doc.deserialize(paths[0].path_, dh);

        size_t max_num_elements = doc.data().size();
        sLOG1 << "COBS_DOC: max_num_elements" << max_num_elements;
        return max_num_elements;
    }
    die("Unknown file type");
}

void construct(const fs::path& in_dir, const fs::path& out_dir,
               uint64_t batch_size, uint64_t num_hashes,
               double false_positive_probability) {
    if (batch_size % 8 != 0)
        die("batch size must be divisible by 8");

    DocumentList in_filelist(in_dir, FileType::Any);

    // estimate signature size by finding number of elements in the largest file
    uint64_t max_num_elements = get_max_file_size(in_filelist);
    uint64_t signature_size = calc_signature_size(
        max_num_elements, num_hashes, false_positive_probability);

    // construct one classic index
    construct_from_documents(
        in_filelist, out_dir / fs::path("1"), signature_size, num_hashes, batch_size);
    size_t i = 1;
    while (!combine(out_dir / fs::path(std::to_string(i)),
                    out_dir / fs::path(std::to_string(i + 1)), batch_size)) {
        i++;
    }

    fs::path index;
    for (fs::directory_iterator sub_it(out_dir.string() + "/" + std::to_string(i + 1)), end; sub_it != end; sub_it++) {
        if (file_has_header<ClassicIndexHeader>(sub_it->path())) {
            index = sub_it->path();
        }
    }
    fs::rename(index, out_dir.string() + "/index" + ClassicIndexHeader::file_extension);
}

void construct_random(const fs::path& out_file,
                      uint64_t signature_size,
                      uint64_t num_documents, size_t document_size,
                      uint64_t num_hashes, size_t seed) {
    Timer t;

    std::vector<std::string> file_names;
    for (size_t i = 0; i < num_documents; ++i) {
        file_names.push_back("file_" + std::to_string(i));
    }

    ClassicIndexHeader cih(signature_size, num_hashes, file_names);
    std::vector<uint8_t> data;
    data.resize(signature_size * cih.block_size());

    std::mt19937 rng(seed);
    KMerBuffer<31> doc;

    t.active("generate");
    for (size_t i = 0; i < num_documents; ++i) {
        cih.file_names()[i] = file_names[i];
        doc.data().clear();
        for (size_t j = 0; j < document_size; ++j) {
            KMer<31> m;
            // should canonicalize the kmer, but since we can assume uniform
            // hashing, this is not needed.
            m.fill_random(rng);
            doc.data().push_back(m);
        }

        std::string term;
        term.reserve(32);

        // TODO: parallel for over setting of bits of the SAME document
        for (uint64_t j = 0; j < doc.data().size(); j++) {
            doc.data()[j].canonicalize();
            doc.data()[j].to_string(&term);
            process_term(term, data, cih, i);
        }
    }

    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "ratio of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    cih.write_file(out_file, data);
    t.stop();

    std::cout << t;
}

} // namespace cobs::classic_index

/******************************************************************************/
