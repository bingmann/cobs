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
#include <tlx/math/round_to_power_of_two.hpp>
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
    uint64_t page_size, uint64_t memory)
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

    LOG1 << "Combine Compact Index from " << paths.size() << " Classic Indices";

    for (size_t i = 0; i < paths.size(); i++) {
        auto h = deserialize_header<ClassicIndexHeader>(paths[i]);
        parameters.push_back({ h.signature_size(), h.num_hashes() });
        file_names.insert(file_names.end(),
                          h.file_names().begin(), h.file_names().end());

        if (term_size == 0) {
            term_size = h.term_size();
            canonicalize = h.canonicalize();
        }
        die_unequal(term_size, h.term_size());
        die_unequal(canonicalize, h.canonicalize());

        LOG1 << i << ": " << h.row_bits() << " documents "
             << tlx::format_iec_units(fs::file_size(paths[i])) << 'B'
             << " row_size " << h.row_size()
             << " : " << paths[i].string();

        if (i < paths.size() - 1) {
            die_unless(h.row_size() == page_size);
        }
        else {
            die_unless(h.row_size() <= page_size);
        }
    }

    Timer t;

    CompactIndexHeader h(term_size, canonicalize, parameters,
                         file_names, page_size);
    std::ofstream ofs;
    serialize_header(ofs, out_file, h);

    for (const auto& p : paths) {
        std::ifstream ifs;
        uint64_t row_size =
            deserialize_header<ClassicIndexHeader>(ifs, p).row_size();
        if (row_size == page_size) {
            // row_size is page_size -> direct copy
            t.active("copy");
            ofs << ifs.rdbuf();
            t.stop();
        }
        else {
            // row_size needs to be padded to page_size
            size_t batch_size = memory / 2 / page_size;

            uint64_t data_size = get_stream_size(ifs);
            batch_size = std::min(
                batch_size, tlx::div_ceil(data_size, page_size));

            sLOG0 << "batch_size" << batch_size;

            std::vector<char> buffer(batch_size* page_size);
            die_unless(data_size % row_size == 0);

            while (data_size > 0) {
                t.active("read");
                size_t this_batch = std::min(batch_size, data_size / row_size);
                ifs.read(buffer.data(), this_batch * row_size);
                die_unequal(this_batch * row_size,
                            static_cast<size_t>(ifs.gcount()));
                data_size -= this_batch * row_size;

                t.active("expand");
                // expand each row_size to page_size, start at the back
                for (size_t b = this_batch; b != 0; ) {
                    --b;

                    std::copy_backward(
                        buffer.begin() + b * row_size,
                        buffer.begin() + (b + 1) * row_size,
                        buffer.begin() + b * page_size + row_size);
                    std::fill(
                        buffer.begin() + b * page_size + row_size,
                        buffer.begin() + (b + 1) * page_size,
                        0);
                }

                t.active("write");
                ofs.write(buffer.data(), this_batch * page_size);
                t.stop();
            }
        }

        ifs.close();
        if (!gopt_keep_temporary) {
            fs::remove(p);
        }
    }
    t.print("compact_combine_into_compact()");
}

void compact_construct(const fs::path& in_dir, const fs::path& index_dir,
                       CompactIndexParameters params) {
    size_t iteration = 1;

    // read file list, sort by size
    DocumentList doc_list(in_dir);
    doc_list.sort_by_size();

    if (params.page_size == 0) {
        params.page_size = tlx::round_up_to_power_of_two(
            static_cast<size_t>(std::sqrt(doc_list.size() / 8)));
        params.page_size = std::max<uint64_t>(params.page_size, 8);
        params.page_size = std::min<uint64_t>(params.page_size, 4096);
    }

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
         << " = " << tlx::format_iec_units(params.mem_bytes) << 'B' << '\n'
         << "  num_threads: " << num_threads;

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

            total_size += params.page_size * signature_size;
        });

    LOG1 << "  total_size: " << tlx::format_iec_units(total_size) << 'B';

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
            classic_params.num_threads =
                tlx::div_ceil(params.num_threads, num_threads);
            classic_params.log_prefix
                = "[" + pad_index(batch_num, 2)
                  + "/" + pad_index(num_pages, 2) + "] ";

            LOG1 << "Classic Sub-Index Parameters: "
                 << classic_params.log_prefix << '\n'
                 << "  number of documents: " << files.size() << '\n'
                 << "  maximum document size: " << max_doc_size << '\n'
                 << "  signature_size: " << signature_size << '\n'
                 << "  sub-index size: "
                 << (docsize_roundup / 8 * signature_size) << " = "
                 << tlx::format_iec_units(docsize_roundup / 8 * signature_size)
                 << '\n'
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
        params.page_size, params.mem_bytes);
}

} // namespace cobs

/******************************************************************************/
