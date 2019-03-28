/*******************************************************************************
 * cobs/construction/compact_index.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/compact_index.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/kmer_buffer_header.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/parameters.hpp>

#include <iomanip>

#include <tlx/die.hpp>

namespace cobs::compact_index {

bool combine_classic_index(const fs::path& in_dir, const fs::path& out_dir, size_t batch_size) {
    bool all_combined = false;
    for (fs::directory_iterator it(in_dir), end; it != end; it++) {
        if (fs::is_directory(it->path())) {
            all_combined = classic_index::combine(
                in_dir / it->path().filename(), out_dir / it->path().filename(), batch_size);
        }
    }
    return all_combined;
}

void combine(const fs::path& in_dir, const fs::path& out_file, uint64_t page_size) {
    std::vector<fs::path> paths;
    fs::recursive_directory_iterator it(in_dir), end;
    std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
                     return file_has_header<ClassicIndexHeader>(p);
                 });
    std::sort(paths.begin(), paths.end());

    std::vector<CompactIndexHeader::parameter> parameters;
    std::vector<std::string> file_names;
    for (size_t i = 0; i < paths.size(); i++) {
        auto h = deserialize_header<ClassicIndexHeader>(paths[i]);
        parameters.push_back({ h.signature_size(), h.num_hashes() });
        file_names.insert(file_names.end(), h.file_names().begin(), h.file_names().end());
        std::cout << i << ": " << h.block_size() << " " << paths[i].string() << std::endl;
        if (i < paths.size() - 1) {
            die_unless(h.block_size() == page_size);
        }
        else {
            die_unless(h.block_size() <= page_size);
        }
    }

    CompactIndexHeader h(parameters, file_names, page_size);
    std::ofstream ofs;
    serialize_header(ofs, out_file, h);

    std::vector<char> buffer(1024 * page_size);
    for (const auto& p : paths) {
        std::ifstream ifs;
        uint64_t block_size = deserialize_header<ClassicIndexHeader>(ifs, p).block_size();
        if (block_size == page_size) {
            ofs << ifs.rdbuf();
        }
        else {
            uint64_t data_size = get_stream_size(ifs);
            std::vector<char> padding(page_size - block_size, 0);
            while (data_size > 0) {
                size_t num_uint8_ts = std::min(1024 * block_size, data_size);
                ifs.read(buffer.data(), num_uint8_ts);
                data_size -= num_uint8_ts;
                for (size_t i = 0; i < num_uint8_ts; i += block_size) {
                    ofs.write(buffer.data() + i, block_size);
                    ofs.write(padding.data(), padding.size());
                }
            }
        }
    }
}

void construct_from_folders(const fs::path& in_dir, const fs::path& index_dir,
                            size_t batch_size, size_t num_hashes,
                            double false_positive_probability, uint64_t page_size) {
    std::string bloom_dir = index_dir.string() + "/cobs_";
    size_t iteration = 1;

    die_unless(batch_size % 8 == 0);

    // read file list, sort by size
    DocumentList doc_list(in_dir, FileType::Any);
    doc_list.sort_by_size();

    // process batches and create classic indexes for each batch
    size_t batch_num = 1;
    doc_list.process_batches(
        8 * page_size,
        [&](const std::vector<DocumentEntry>& files, fs::path /* out_file */) {

            size_t max_file_size = 0;
            for (const DocumentEntry& de : files)
                max_file_size = std::max(max_file_size, de.size_);

            DocumentList batch_list(files);
            fs::path classic_dir = bloom_dir + pad_index(iteration);

            size_t signature_size = calc_signature_size(
                max_file_size / 8, num_hashes, false_positive_probability);
            classic_index::construct_from_documents(
                batch_list, classic_dir / pad_index(batch_num),
                signature_size, num_hashes, batch_size);

            batch_num++;
        });

    // combine classic indexes
    while (!combine_classic_index(bloom_dir + pad_index(iteration),
                                  bloom_dir + pad_index(iteration + 1),
                                  batch_size)) {
        iteration++;
    }

    // combine classic indexes into one compact index
    combine(bloom_dir + pad_index(iteration + 1),
            index_dir / ("index" + CompactIndexHeader::file_extension),
            page_size);
}

} // namespace cobs::compact_index

/******************************************************************************/
