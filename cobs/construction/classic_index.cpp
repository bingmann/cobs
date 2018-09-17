/*******************************************************************************
 * cobs/construction/classic_index.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <xxhash.h>

#include <cobs/construction/classic_index.hpp>
#include <cobs/kmer.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

namespace cobs::classic_index {

void create_hashes(const void* input, size_t len, uint64_t signature_size,
                   uint64_t num_hashes,
                   const std::function<void(uint64_t)>& callback) {
    for (unsigned int i = 0; i < num_hashes; i++) {
        uint64_t hash = XXH32(input, len, i);
        callback(hash % signature_size);
    }
}

void set_bit(std::vector<uint8_t>& data, const cobs::file::classic_index_header& h, uint64_t pos, uint64_t bit_in_block) {
    data[h.block_size() * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
}

void process(const std::vector<fs::path>& paths,
             const fs::path& out_file, std::vector<uint8_t>& data,
             cobs::file::classic_index_header& h, timer& t) {

    sample<31> s;
    cobs::file::sample_header sh;
    for (uint64_t i = 0; i < paths.size(); i++) {
        t.active("read");
        file::deserialize(paths[i], s, sh);
        h.file_names()[i] = sh.name();
        t.active("process");

#pragma omp parallel for
        for (uint64_t j = 0; j < s.data().size(); j++) {
            create_hashes(s.data().data() + j, 8, h.signature_size(), h.num_hashes(), [&](uint64_t hash) {
                              set_bit(data, h, hash, i);
                          });
        }
    }
    t.active("write");
    file::serialize(out_file, data, h);
    t.stop();
}

void combine(std::vector<std::pair<std::ifstream, uint64_t> >& ifstreams,
             const fs::path& out_file,
             uint64_t signature_size, uint64_t block_size, uint64_t num_hash,
             timer& t, const std::vector<std::string>& file_names) {
    std::ofstream ofs;
    file::classic_index_header h(signature_size, num_hash, file_names);
    file::serialize_header(ofs, out_file, h);

    std::vector<char> block(block_size);
    for (uint64_t i = 0; i < signature_size; i++) {
        uint64_t pos = 0;
        t.active("read");
        for (auto& ifs : ifstreams) {
            ifs.first.read(&(*(block.begin() + pos)), ifs.second);
            pos += ifs.second;
        }
        t.active("write");
        ofs.write(&(*block.begin()), block_size);
    }
    t.stop();
}

void create_from_samples(const fs::path& in_dir, const fs::path& out_dir,
                         uint64_t signature_size, uint64_t num_hashes,
                         uint64_t batch_size) {
    timer t;
    cobs::file::classic_index_header h(signature_size, num_hashes);
    std::vector<uint8_t> data;
    bulk_process_files<file::sample_header>(
        in_dir, out_dir, batch_size, file::classic_index_header::file_extension,
        [&](const std::vector<fs::path>& paths, const fs::path& out_file) {
            h.file_names().resize(paths.size());
            data.resize(signature_size * h.block_size());
            std::fill(data.begin(), data.end(), 0);
            process(paths, out_file, data, h, t);
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
        bulk_process_files<file::classic_index_header>(
            in_dir, out_dir, batch_size, file::classic_index_header::file_extension,
            [&](const std::vector<fs::path>& paths, const fs::path& out_file) {
                uint64_t new_block_size = 0;
                for (size_t i = 0; i < paths.size(); i++) {
                    ifstreams.emplace_back(std::make_pair(std::ifstream(), 0));
                    auto h = file::deserialize_header<file::classic_index_header>(ifstreams.back().first, paths[i]);
                    if (signature_size == 0) {
                        signature_size = h.signature_size();
                        num_hashes = h.num_hashes();
                    }
                    assert(h.signature_size() == signature_size);
                    assert(h.num_hashes() == num_hashes);
#ifndef isi_test
                    if (i < paths.size() - 2) {
                        // todo doesnt work because of padding for compact, which means there could be two files with less file_names
                        // todo quickfix with -2 to allow for paddding
                        // assert(h.file_names().size() == 8 * h.block_size());
                    }
#endif
                    ifstreams.back().second = h.block_size();
                    new_block_size += h.block_size();
                    std::copy(h.file_names().begin(), h.file_names().end(), std::back_inserter(file_names));
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
    bulk_process_files<file::sample_header>(
        in_dir, out_dir, UINT64_MAX, file::sample_header::file_extension,
        [&](std::vector<fs::path>& paths, const fs::path& /*out_file*/) {
            std::sort(paths.begin(), paths.end(),
                      [](const auto& p1, const auto& p2) {
                          return fs::file_size(p1) > fs::file_size(p2);
                      });
            size_t max_num_elements = fs::file_size(paths[0]) / 8;
            signature_size = cobs::calc_signature_size(max_num_elements, num_hashes, false_positive_probability);
        });
    return signature_size;
}

void create(const fs::path& in_dir, const fs::path& out_dir,
            uint64_t batch_size, uint64_t num_hashes, double false_positive_probability) {
    assert_throw<std::invalid_argument>(batch_size % 8 == 0, "batch size must be divisible by 8");
    uint64_t signature_size = get_signature_size(in_dir, out_dir, num_hashes, false_positive_probability);
    create_from_samples(in_dir, out_dir / fs::path("1"), signature_size, num_hashes, batch_size);
    size_t i = 1;
    while (!combine(out_dir / fs::path(std::to_string(i)),
                    out_dir / fs::path(std::to_string(i + 1)), batch_size)) {
        i++;
    }

    fs::path index;
    for (fs::directory_iterator sub_it(out_dir.string() + "/" + std::to_string(i + 1)), end; sub_it != end; sub_it++) {
        if (cobs::file::file_is<cobs::file::classic_index_header>(sub_it->path())) {
            index = sub_it->path();
        }
    }
    fs::rename(index, out_dir.string() + "/index" + cobs::file::classic_index_header::file_extension);
}

void create_dummy(const fs::path& p, uint64_t signature_size, uint64_t block_size, uint64_t num_hashes) {
    std::vector<std::string> file_names;
    for (size_t i = 0; i < 8 * block_size; i++) {
        file_names.push_back("file_" + std::to_string(i));
    }
    cobs::file::classic_index_header h(signature_size, num_hashes, file_names);
    std::ofstream ofs;
    cobs::file::serialize_header(ofs, p, h);

    for (size_t i = 0; i < signature_size * block_size; i += 4) {
        int rnd = std::rand();
        ofs.write(reinterpret_cast<char*>(&rnd), 4);
    }
}

} // namespace cobs::classic_index

/******************************************************************************/
