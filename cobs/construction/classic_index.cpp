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
#include <cobs/cortex.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/kmer.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

#include <tlx/logger.hpp>
#include <tlx/math/popcount.hpp>

namespace cobs::classic_index {

void set_bit(std::vector<uint8_t>& data,
             const file::classic_index_header& cih,
             uint64_t pos, uint64_t bit_in_block) {
    data[cih.block_size() * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
}

void process(const std::vector<fs::path>& paths,
             const fs::path& out_file, std::vector<uint8_t>& data,
             file::classic_index_header& cih, timer& t) {

    document<31> doc;
    file::document_header dh;
    for (uint64_t i = 0; i < paths.size(); i++) {
        if (paths[i].extension() == ".ctx") {
            t.active("read");
            cortex::cortex_file ctx(paths[i].string());
            cih.file_names()[i] = ctx.name_;
            doc.data().clear();
            ctx.process_kmers<31>(
                [&](const kmer<31>& m) { doc.data().push_back(m); });

#pragma omp parallel for
            for (uint64_t j = 0; j < doc.data().size(); j++) {
                process_hashes(doc.data().data() + j, 8,
                               cih.signature_size(), cih.num_hashes(),
                               [&](uint64_t hash) {
                                   set_bit(data, cih, hash, i);
                               });
            }
        }
        else if (paths[i].extension() == ".isi") {
            t.active("read");
            doc.deserialize(paths[i], dh);
            cih.file_names()[i] = dh.name();
            t.active("process");

#pragma omp parallel for
            for (uint64_t j = 0; j < doc.data().size(); j++) {
                process_hashes(doc.data().data() + j, 8,
                               cih.signature_size(), cih.num_hashes(),
                               [&](uint64_t hash) {
                                   set_bit(data, cih, hash, i);
                               });
            }
        }
    }
    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "percent of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    cih.write_file(out_file, data);
    t.stop();
}

void combine(std::vector<std::pair<std::ifstream, uint64_t> >& ifstreams,
             const fs::path& out_file,
             uint64_t signature_size, uint64_t block_size, uint64_t num_hash,
             timer& t, const std::vector<std::string>& file_names) {
    std::ofstream ofs;
    file::classic_index_header cih(signature_size, num_hash, file_names);
    file::serialize_header(ofs, out_file, cih);

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

void construct_from_documents(const fs::path& in_dir, const fs::path& out_dir,
                              uint64_t signature_size, uint64_t num_hashes,
                              uint64_t batch_size) {
    timer t;
    file::classic_index_header cih(signature_size, num_hashes);
    std::vector<uint8_t> data;
    process_file_batches(
        in_dir, out_dir, batch_size, file::classic_index_header::file_extension,
        [](const fs::path& path) {
            return path.extension() == ".ctx" || path.extension() == ".isi";
        },
        [&](const std::vector<fs::path>& paths, const fs::path& out_file) {
            cih.file_names().resize(paths.size());
            data.resize(signature_size * cih.block_size());
            std::fill(data.begin(), data.end(), 0);
            process(paths, out_file, data, cih, t);
        });
    std::cout << t;
}

bool combine(const fs::path& in_dir, const fs::path& out_dir, uint64_t batch_size) {
    timer t;
    std::vector<std::pair<std::ifstream, uint64_t> > ifstreams;
    std::vector<std::string> file_names;
    uint64_t signature_size = 0;
    uint64_t num_hashes = 0;
    bool all_combined =
        process_file_batches(
            in_dir, out_dir, batch_size, file::classic_index_header::file_extension,
            [](const fs::path& path) {
                return file::file_is<file::classic_index_header>(path);
            },
            [&](const std::vector<fs::path>& paths, const fs::path& out_file) {
                uint64_t new_block_size = 0;
                for (size_t i = 0; i < paths.size(); i++) {
                    ifstreams.emplace_back(std::make_pair(std::ifstream(), 0));
                    auto cih = file::deserialize_header<file::classic_index_header>(ifstreams.back().first, paths[i]);
                    if (signature_size == 0) {
                        signature_size = cih.signature_size();
                        num_hashes = cih.num_hashes();
                    }
                    assert(cih.signature_size() == signature_size);
                    assert(cih.num_hashes() == num_hashes);
#ifndef isi_test
                    if (i < paths.size() - 2) {
                        // todo doesnt work because of padding for compact, which means there could be two files with less file_names
                        // todo quickfix with -2 to allow for paddding
                        // assert(cih.file_names().size() == 8 * cih.block_size());
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

uint64_t get_signature_size(const fs::path& in_dir, const fs::path& out_dir,
                            uint64_t num_hashes, double false_positive_probability) {
    uint64_t signature_size;
    process_file_batches(
        in_dir, out_dir, UINT64_MAX, file::document_header::file_extension,
        [](const fs::path& path) {
            return path.extension() == ".ctx" || path.extension() == ".isi";
        },
        [&](std::vector<fs::path>& paths, const fs::path& /* out_file */) {
            std::sort(paths.begin(), paths.end(),
                      [](const auto& p1, const auto& p2) {
                          return fs::file_size(p1) > fs::file_size(p2);
                      });
            if (paths[0].extension() == ".ctx") {
                cortex::cortex_file ctx(paths[0].string());
                size_t max_num_elements = ctx.num_documents();
                sLOG1 << "CTX: max_num_elements" << max_num_elements;
                signature_size = calc_signature_size(
                    max_num_elements, num_hashes, false_positive_probability);
            }
            else if (paths[0].extension() == ".isi") {
                file::document_header dh;
                document<31> doc;
                doc.deserialize(paths[0], dh);

                size_t max_num_elements = doc.data().size();
                sLOG1 << "ISI: max_num_elements" << max_num_elements;
                signature_size = calc_signature_size(
                    max_num_elements, num_hashes, false_positive_probability);
            }
        });
    return signature_size;
}

void construct(const fs::path& in_dir, const fs::path& out_dir,
               uint64_t batch_size, uint64_t num_hashes,
               double false_positive_probability) {
    assert_throw<std::invalid_argument>(
        batch_size % 8 == 0, "batch size must be divisible by 8");
    uint64_t signature_size =
        get_signature_size(in_dir, out_dir, num_hashes, false_positive_probability);
    construct_from_documents(
        in_dir, out_dir / fs::path("1"), signature_size, num_hashes, batch_size);
    size_t i = 1;
    while (!combine(out_dir / fs::path(std::to_string(i)),
                    out_dir / fs::path(std::to_string(i + 1)), batch_size)) {
        i++;
    }

    fs::path index;
    for (fs::directory_iterator sub_it(out_dir.string() + "/" + std::to_string(i + 1)), end; sub_it != end; sub_it++) {
        if (file::file_is<file::classic_index_header>(sub_it->path())) {
            index = sub_it->path();
        }
    }
    fs::rename(index, out_dir.string() + "/index" + file::classic_index_header::file_extension);
}

void construct_random(const fs::path& out_file,
                      uint64_t signature_size,
                      uint64_t num_documents, size_t document_size,
                      uint64_t num_hashes, size_t seed) {
    timer t;

    std::vector<std::string> file_names;
    for (size_t i = 0; i < num_documents; ++i) {
        file_names.push_back("file_" + std::to_string(i));
    }

    file::classic_index_header cih(signature_size, num_hashes, file_names);
    std::vector<uint8_t> data;
    data.resize(signature_size * cih.block_size());

    std::mt19937 rng(seed);
    document<31> doc;

    t.active("generate");
    for (size_t i = 0; i < num_documents; ++i) {
        cih.file_names()[i] = file_names[i];
        doc.data().clear();
        for (size_t j = 0; j < document_size; ++j) {
            kmer<31> m;
            // should canonicalize the kmer, but since we can assume uniform
            // hashing, this is not needed.
            m.fill_random(rng);
            doc.data().push_back(m);
        }

#pragma omp parallel for
        for (size_t j = 0; j < doc.data().size(); ++j) {
            process_hashes(doc.data().data() + j, 8,
                           cih.signature_size(), cih.num_hashes(),
                           [&](uint64_t hash) {
                               set_bit(data, cih, hash, i);
                           });
        }
    }

    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "percent of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    cih.write_file(out_file, data);
    t.stop();

    std::cout << t;
}

} // namespace cobs::classic_index

/******************************************************************************/
