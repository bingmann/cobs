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
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/file.hpp>

#include <iomanip>

#include <tlx/die.hpp>
#include <tlx/math/div_ceil.hpp>
#include <tlx/math/round_up.hpp>
#include <tlx/string/format_iec_units.hpp>

namespace cobs {

bool combine_classic_index(const fs::path& in_dir, const fs::path& out_dir,
                           size_t mem_bytes, size_t num_threads) {
    bool all_combined = false;
    for (fs::directory_iterator it(in_dir), end; it != end; it++) {
        if (fs::is_directory(it->path())) {
            all_combined = classic_combine(
                in_dir / it->path().filename(),
                out_dir / it->path().filename(),
                mem_bytes, num_threads);
        }
    }
    return all_combined;
}

void compact_combine_into_compact(
    const fs::path& in_dir, const fs::path& out_file,
    uint64_t page_size)
{
    std::vector<fs::path> paths;
    fs::recursive_directory_iterator it(in_dir), end;
    std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
                     return file_has_header<ClassicIndexHeader>(p);
                 });
    std::sort(paths.begin(), paths.end());

    unsigned term_size = 0;
    uint8_t canonicalize = 0;
    std::vector<CompactIndexHeader::parameter> parameters;
    std::vector<std::string> file_names;

    for (size_t i = 0; i < paths.size(); i++) {
        auto h = deserialize_header<ClassicIndexHeader>(paths[i]);
        parameters.push_back({ h.signature_size(), h.num_hashes() });
        file_names.insert(file_names.end(), h.file_names().begin(), h.file_names().end());

        if (term_size == 0) {
            term_size = h.term_size();
            canonicalize = h.canonicalize();
        }
        die_unequal(term_size, h.term_size());
        die_unequal(canonicalize, h.canonicalize());

        LOG1 << i << ": " << h.row_size() << " " << paths[i].string();

        if (i < paths.size() - 1) {
            die_unless(h.row_size() == page_size);
        }
        else {
            die_unless(h.row_size() <= page_size);
        }
    }

    CompactIndexHeader h(term_size, canonicalize, parameters, file_names, page_size);
    std::ofstream ofs;
    serialize_header(ofs, out_file, h);

    std::vector<char> buffer(1024 * page_size);
    for (const auto& p : paths) {
        std::ifstream ifs;
        uint64_t row_size = deserialize_header<ClassicIndexHeader>(ifs, p).row_size();
        if (row_size == page_size) {
            ofs << ifs.rdbuf();
        }
        else {
            uint64_t data_size = get_stream_size(ifs);
            std::vector<char> padding(page_size - row_size, 0);
            while (data_size > 0) {
                size_t num_uint8_ts = std::min(1024 * row_size, data_size);
                ifs.read(buffer.data(), num_uint8_ts);
                data_size -= num_uint8_ts;
                for (size_t i = 0; i < num_uint8_ts; i += row_size) {
                    ofs.write(buffer.data() + i, row_size);
                    ofs.write(padding.data(), padding.size());
                }
            }
        }
    }
}

void compact_construct(const fs::path& in_dir, const fs::path& index_dir,
                       CompactIndexParameters params) {
    size_t iteration = 1;

    // read file list, sort by size
    DocumentList doc_list(in_dir);
    doc_list.sort_by_size();

    size_t num_pages = tlx::div_ceil(doc_list.size(), 8 * params.page_size);

    size_t num_threads = params.num_threads;
    if (num_threads > num_pages) {
        // use div_floor() instead
        num_threads = doc_list.size() / (8 * params.page_size);
    }
    if (num_threads == 0) num_threads = 1;

    LOG1 << "Compact Index Parameters:\n"
         << "  term_size: " << params.term_size << '\n'
         << "  number of documents: " << doc_list.size() << '\n'
         << "  num_hashes: " << params.num_hashes << '\n'
         << "  false_positive_rate: " << params.false_positive_rate << '\n'
         << "  page_size: " << params.page_size << " bytes"
         << " = " << params.page_size * 8 << " documents" << '\n'
         << "  num_pages: " << num_pages << '\n'
         << "  mem_bytes: " << params.mem_bytes
         << " = " << tlx::format_iec_units(params.mem_bytes);

    size_t total_size = 0;

    doc_list.process_batches(
        8 * params.page_size,
        [&](size_t /* batch_num */, const std::vector<DocumentEntry>& files,
            fs::path /* out_file */) {

            size_t max_doc_size = 0;
            for (const DocumentEntry& de : files) {
                max_doc_size = std::max(
                    max_doc_size, de.num_terms(params.term_size));
            }

            size_t signature_size = calc_signature_size(
                max_doc_size, params.num_hashes, params.false_positive_rate);

            size_t docsize_roundup = tlx::round_up(files.size(), 8);

            total_size += docsize_roundup / 8 * signature_size;
        });

    LOG1 << "  total_size: " << tlx::format_iec_units(total_size);

    // process batches and create classic indexes for each batch
    doc_list.process_batches_parallel(
        8 * params.page_size, num_threads,
        [&](size_t batch_num, const std::vector<DocumentEntry>& files,
            fs::path /* out_file */) {

            size_t max_doc_size = 0;
            for (const DocumentEntry& de : files) {
                max_doc_size = std::max(
                    max_doc_size, de.num_terms(params.term_size));
            }

            size_t signature_size = calc_signature_size(
                max_doc_size, params.num_hashes, params.false_positive_rate);

            size_t docsize_roundup = tlx::round_up(files.size(), 8);

            if (max_doc_size == 0)
                return;

            ClassicIndexParameters classic_params;
            classic_params.term_size = params.term_size;
            classic_params.canonicalize = params.canonicalize;
            classic_params.num_hashes = params.num_hashes;
            classic_params.false_positive_rate = params.false_positive_rate;
            classic_params.signature_size = signature_size;
            classic_params.mem_bytes = params.mem_bytes / num_threads;
            classic_params.num_threads = params.num_threads / num_threads;

            LOG1 << "Classic Sub-Index Parameters: "
                 << "[" << batch_num << '/' << num_pages << ']' << '\n'
                 << "  number of documents: " << files.size() << '\n'
                 << "  maximum document size: " << max_doc_size << '\n'
                 << "  signature_size: " << signature_size << '\n'
                 << "  sub-index size: "
                 << (docsize_roundup / 8 * signature_size) << '\n'
                 << "  mem_bytes: " << classic_params.mem_bytes << '\n'
                 << "  num_threads: " << classic_params.num_threads;

            DocumentList batch_list(files);
            fs::path classic_dir = index_dir / pad_index(iteration);

            classic_construct_from_documents(
                batch_list, classic_dir / pad_index(batch_num), classic_params);
        });

    // combine classic indexes
    while (!combine_classic_index(index_dir / pad_index(iteration),
                                  index_dir / pad_index(iteration + 1),
                                  params.mem_bytes, params.num_threads)) {
        iteration++;
    }

    // combine classic indexes into one compact index
    compact_combine_into_compact(
        index_dir / pad_index(iteration + 1),
        index_dir / ("index" + CompactIndexHeader::file_extension),
        params.page_size);
}

} // namespace cobs

/******************************************************************************/
