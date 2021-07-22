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
#include <cobs/util/calc_signature_size.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/process_file_batches.hpp>
#include <cobs/util/timer.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>
#include <tlx/math/div_ceil.hpp>
#include <tlx/math/popcount.hpp>
#include <tlx/math/round_up.hpp>
#include <tlx/string/bitdump.hpp>
#include <tlx/string/format_iec_units.hpp>
#include <tlx/string/join_generic.hpp>

namespace cobs {

/******************************************************************************/
// Construction of classic index from documents

static inline
void set_bit(std::vector<uint8_t>& data, const ClassicIndexHeader& cih,
             uint64_t pos, uint64_t doc_index) {
    data[cih.row_size() * pos + doc_index / 8] |= 1 << (doc_index % 8);
}

static inline
void process_term(const tlx::string_view& term, std::vector<uint8_t>& data,
                  size_t doc_index, const std::string& path,
                  bool* shown_canonicalization_warning,
                  const ClassicIndexHeader& cih, char* canonicalize_buffer) {
    if (cih.canonicalize_ == 0) {
        process_hashes(term.data(), term.size(),
                       cih.signature_size_, cih.num_hashes_,
                       [&](uint64_t hash) {
                           set_bit(data, cih, hash, doc_index);
                       });
    }
    else if (cih.canonicalize_ == 1) {
        bool good =
            canonicalize_kmer(term.data(), canonicalize_buffer, term.size());

        if (!good && !*shown_canonicalization_warning) {
            LOG1 << "WARNING: Invalid DNA base pair (not ACGT) in document: "
                 << path;
            *shown_canonicalization_warning = true;
        }

        process_hashes(canonicalize_buffer, term.size(),
                       cih.signature_size_, cih.num_hashes_,
                       [&](uint64_t hash) {
                           set_bit(data, cih, hash, doc_index);
                       });
    }
}

static inline
void process_batch(size_t batch_num, size_t num_batches, size_t num_threads,
                   std::string log_prefix,
                   const std::vector<DocumentEntry>& paths,
                   const fs::path& out_file,
                   ClassicIndexHeader& cih, Timer& t) {

    LOG1 << log_prefix
         << pad_index(batch_num) << '/' << pad_index(num_batches)
         << " documents " << paths.size()
         << " row_size " << cih.row_size()
         << " signature_size " << cih.signature_size_
         << " matrix_size " << cih.signature_size_ * cih.row_size() << " = "
         << tlx::format_iec_units(cih.signature_size_ * cih.row_size()) << 'B';

    die_unless(paths.size() <= cih.row_size() * 8);
    std::vector<uint8_t> data(cih.signature_size_* cih.row_size());

    std::atomic<size_t> count = 0;
    t.active("process");

    // parallelize over 8 documents, which fit into one byte, the terms are
    // random hence only little cache trashing inside a cache line should occur.
    parallel_for(
        0, (paths.size() + 7) / 8, num_threads,
        [&](size_t b) {
            tlx::simple_vector<char> canonicalize_buffer(cih.term_size_);

            size_t local_count = 0;
            for (size_t i = 8 * b; i < 8 * (b + 1) && i < paths.size(); ++i) {
                cih.file_names_[i] = paths[i].name_;
                bool shown_canonicalization_warning = false;

                paths[i].process_terms(
                    cih.term_size_,
                    [&](const tlx::string_view& term) {
                        process_term(term, data, i, paths[i].path_,
                                     &shown_canonicalization_warning,
                                     cih, canonicalize_buffer.data());
                        ++local_count;
                    });
            }
            count += local_count;
        });

    t.active("write");
    cih.write_file(out_file, data);

    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << log_prefix
         << pad_index(batch_num) << '/' << pad_index(num_batches)
         << " done: terms " << count << " ratio_of_ones "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.stop();
}

void classic_construct_from_documents(
    const DocumentList& doc_list, const fs::path& out_dir,
    const ClassicIndexParameters& params)
{
    Timer t;
    fs::create_directories(out_dir);

    size_t num_threads = params.num_threads;
    if (num_threads == 0)
        num_threads = 1;

    size_t batch_size =
        params.mem_bytes / (params.signature_size / 8) / num_threads;
    batch_size = std::max(8lu, tlx::round_up(batch_size, 8));
    num_threads = std::min(
        num_threads,
        params.mem_bytes / (batch_size / 8 * params.signature_size));

    die_unless(params.num_hashes != 0);
    die_unless(params.signature_size != 0);
    die_unless(batch_size % 8 == 0);

    size_t num_batches = tlx::div_ceil(doc_list.size(), batch_size);

    LOG1 << params.log_prefix
         << "classic_construct_from_documents()"
         << " batch_size=" << batch_size
         << " num_threads=" << num_threads
         << " num_batches=" << num_batches;

    doc_list.process_batches_parallel(
        batch_size, num_threads,
        [&](size_t batch_num, const std::vector<DocumentEntry>& paths,
            std::string out_file) {
            Timer thr_timer;

            LOG1 << params.log_prefix
                 << "Construct Classic Index " << out_file;

            fs::path out_path =
                out_dir / (out_file + ClassicIndexHeader::file_extension);
            if (fs::exists(out_path))
                return;

            ClassicIndexHeader cih;
            cih.term_size_ = params.term_size;
            cih.canonicalize_ = params.canonicalize;
            cih.signature_size_ = params.signature_size;
            cih.num_hashes_ = params.num_hashes;
            cih.file_names_.resize(paths.size());
            process_batch(batch_num, num_batches,
                          tlx::div_ceil(num_threads, num_batches),
                          params.log_prefix, paths, out_path, cih, thr_timer);

            t += thr_timer;
        });
    t.print("classic_construct_from_documents");
}

/******************************************************************************/
// Combine multiple classic indexes

static inline
void classic_combine_streams(
    std::vector<std::ifstream>& streams,
    std::vector<uint64_t>& row_bits,
    const fs::path& out_file,
    unsigned term_size, uint8_t canonicalize, uint64_t signature_size,
    uint64_t new_row_bits, uint64_t num_hash, uint64_t mem_bytes,
    Timer& t, const std::vector<std::string>& file_names)
{
    static constexpr bool debug = false;

    if (fs::exists(out_file))
        return;

    std::ofstream ofs;
    ClassicIndexHeader cih;
    cih.term_size_ = term_size;
    cih.canonicalize_ = canonicalize;
    cih.signature_size_ = signature_size;
    cih.num_hashes_ = num_hash;
    cih.file_names_ = file_names;
    serialize_header(ofs, out_file, cih);

    die_unequal(streams.size(), row_bits.size());
    die_unequal(new_row_bits, file_names.size());

    // use fast method if all row_bits are zero mod 8 with exception of last one
    bool use_fast_method = true;

    size_t in_row_bytes = 0;
    std::vector<uint64_t> row_bytes(streams.size());
    for (size_t i = 0; i < streams.size(); ++i) {
        if (i + 1 != streams.size() && row_bits[i] % 8 != 0)
            use_fast_method = false;

        row_bytes[i] = tlx::div_ceil(row_bits[i], 8);
        in_row_bytes += row_bytes[i];
    }

    size_t new_row_bytes = tlx::div_ceil(new_row_bits, 8);
    size_t batch_size = mem_bytes / new_row_bytes / 2;
    if (batch_size < 1) batch_size = 1;

    size_t num_batches = tlx::div_ceil(signature_size, batch_size);

    LOG1 << "classic_combine_streams()"
         << " streams=" << streams.size()
         << " new_row_bits=" << new_row_bits
         << " new_row_bytes=" << new_row_bytes
         << " batch_size=" << batch_size
         << " num_batches=" << num_batches
         << " use_fast_method=" << use_fast_method
         << " memory=" << ((new_row_bytes + in_row_bytes) * batch_size);

    // read many blocks from each file, interleave them into new block, and
    // write it out
    std::vector<std::vector<unsigned char> > in_blocks(streams.size());
    for (size_t i = 0; i < streams.size(); ++i) {
        in_blocks[i].resize(row_bytes[i] * batch_size);
    }

    std::vector<size_t> in_pos(streams.size());
    // output block
    std::vector<char> out_block(new_row_bytes* batch_size);

    uint64_t current_row = 0;
    for (size_t b = 0; b < num_batches; ++b) {
        t.active("read");
        LOG << "read batch " << b << "/" << num_batches;
        size_t this_batch =
            std::min(batch_size, signature_size - current_row);

        // read data from streams
        for (size_t i = 0; i < streams.size(); ++i) {
            streams[i].read(
                    (char*)(in_blocks[i].data()), row_bytes[i] * this_batch);
            LOG << "stream[" << i << "] read " << streams[i].gcount();
            die_unequal(row_bytes[i] * this_batch,
                        static_cast<size_t>(streams[i].gcount()));
        }
        current_row += this_batch;

        std::fill(in_pos.begin(), in_pos.end(), 0);

        // interleave rows, two methods: one byte aligned, other can use any bit
        // combination.
        t.active("interleave");
        for (size_t k = 0; k < this_batch; ++k) {
            std::vector<char>::iterator out =
                out_block.begin() + k * new_row_bytes;

            if (use_fast_method) {
                // out_pos is in bytes, copies whole bytes.
                size_t out_pos = 0;
                for (size_t s = 0; s < streams.size(); ++s) {
                    LOG << "in[" << s << "] " << tlx::bitdump_le8(
                        in_blocks[s].data() + in_pos[s], row_bytes[s]);

                    for (size_t i = 0; i < row_bytes[s]; ++i) {
                        out[out_pos++] = in_blocks[s][in_pos[s] + i];
                    }
                    in_pos[s] += row_bytes[s];
                }
            }
            else {
                // slower method which can interleave any bit combinations,
                // out_pos is in bits.
                size_t out_pos = 0;
                for (size_t s = 0; s < streams.size(); ++s) {
                    LOG << "in[" << s << "] " << tlx::bitdump_le8(
                        in_blocks[s].data() + in_pos[s], row_bytes[s]);

                    size_t j = row_bits[s];
                    for (size_t i = 0; i < row_bytes[s]; ++i) {
                        out[out_pos / 8] |=
                            in_blocks[s][in_pos[s] + i] << (out_pos % 8);
                        if (j >= (8 - out_pos % 8)) {
                            out[out_pos / 8 + 1] |=
                                in_blocks[s][in_pos[s] + i] >> (8 - out_pos % 8);
                        }
                        out_pos += std::min<size_t>(8, row_bits[s] - 8 * i);
                        j -= 8;
                    }
                    in_pos[s] += row_bytes[s];
                }
            }
            LOG << "out[] " << tlx::bitdump_le8(&*out, new_row_bytes);
        }

        t.active("write");
        ofs.write(out_block.data(), new_row_bytes * this_batch);
        std::fill(out_block.begin(), out_block.end(), '\0');
    }
    t.stop();
}

bool classic_combine(const fs::path& in_dir, const fs::path& out_dir,
                     fs::path& result_file,
                     uint64_t mem_bytes, size_t num_threads,
                     bool keep_temporary)
{
    fs::create_directories(out_dir);

    // ---[ Collect Classic Indexes to Combine ]--------------------------------

    struct Index {
        fs::path path;
        ClassicIndexHeader header;
    };

    std::vector<Index> index_list;

    fs::recursive_directory_iterator it(in_dir), end;
    while (it != end) {
        if (it->path().extension() != ClassicIndexHeader::file_extension) {
            ++it;
            continue;
        }

        auto cih = deserialize_header<ClassicIndexHeader>(*it);
        index_list.emplace_back(Index { *it, cih });
        ++it;
    }

    // handle easy cases
    if (index_list.size() == 0)
        die("classic_combine() could not find any cobs_classic to combine");

    if (index_list.size() == 1) {
        fs::path out_path = out_dir / index_list[0].path.filename();
        LOG1 << "Move 1 Classic Index [" << index_list[0].header.row_bits()
             << " documents] to " << out_path;

        if (!keep_temporary) {
            fs::rename(index_list[0].path, out_path);
            fs::remove(in_dir);
        }
        else {
            fs::copy(index_list[0].path, out_path);
        }

        result_file = out_path;
        return true;
    }

    std::sort(index_list.begin(), index_list.end(),
              [](const Index& a, const Index& b) {
                  return a.path < b.path;
              });

    // ---[ Determine Batches to Combine ]--------------------------------------

    size_t target_row_bits = 8 * mem_bytes / num_threads;

    struct Batch {
        std::vector<fs::path> files;
        std::string out_file;
    };
    std::vector<Batch> batch_list;

    {
        size_t new_row_bits = 0;

        std::vector<fs::path> batch;

        for (size_t i = 0; i < index_list.size(); i++) {
            if (!batch.empty() && (
                    // row buffer exceeds memory
                    new_row_bits + index_list[i].header.row_bits()
                    > target_row_bits ||
                    // only open about 512 files
                    batch.size() > 512 / num_threads))
            {
                std::string out_file = pad_index(batch_list.size());

                batch_list.emplace_back(Batch { std::move(batch), out_file });

                batch.clear();
                new_row_bits = 0;
            }

            std::string filename = cobs::base_name(index_list[i].path);

            batch.push_back(std::move(index_list[i].path));
            new_row_bits += index_list[i].header.row_bits();
        }
        if (!batch.empty()) {
            std::string out_file = pad_index(batch_list.size());
            batch_list.emplace_back(Batch { std::move(batch), out_file });
        }
    }

    Timer t;
    parallel_for(
        0, batch_list.size(), num_threads,
        [&](size_t b) {
            Timer thr_timer;
            const std::vector<fs::path>& files = batch_list[b].files;

            fs::path out_path =
                out_dir /
                (batch_list[b].out_file + ClassicIndexHeader::file_extension);

            if (files.size() == 1) {
                LOG1 << "Move Classic Index to " << out_path;

                if (!keep_temporary)
                    fs::rename(files[0], out_path);
                else
                    fs::copy(files[0], out_path);

                return;
            }

            std::vector<std::ifstream> streams;
            std::vector<uint64_t> row_bits;
            std::vector<std::string> file_names;
            unsigned term_size = 0;
            uint8_t canonicalize = false;
            uint64_t signature_size = 0;
            uint64_t num_hashes = 0;

            streams.reserve(files.size());
            row_bits.reserve(files.size());

            // collect new block size
            uint64_t new_row_bits = 0;
            for (size_t i = 0; i < files.size(); i++) {
                // read header from classic index
                streams.emplace_back(std::ifstream());
                auto cih = deserialize_header<ClassicIndexHeader>(
                    streams.back(), files[i]);
                die_unless(streams.back().good());
                // check parameters for compatibility
                if (signature_size == 0) {
                    term_size = cih.term_size_;
                    canonicalize = cih.canonicalize_;
                    signature_size = cih.signature_size_;
                    num_hashes = cih.num_hashes_;
                }
                die_unequal(cih.term_size_, term_size);
                die_unequal(cih.canonicalize_, canonicalize);
                die_unequal(cih.signature_size_, signature_size);
                die_unequal(cih.num_hashes_, num_hashes);
                // calculate new row length
                row_bits.emplace_back(cih.row_bits());
                new_row_bits += cih.row_bits();
                // append file names
                std::copy(cih.file_names_.begin(), cih.file_names_.end(),
                          std::back_inserter(file_names));
            }

            LOG1 << "Combine Classic " << files.size() << " indices "
                 << "[" << tlx::join(" ", row_bits) << " documents]"
                 << " into " << out_path;

            classic_combine_streams(
                streams, row_bits, out_path, term_size, canonicalize,
                signature_size, new_row_bits, num_hashes,
                mem_bytes / num_threads, thr_timer, file_names);
            streams.clear();
            file_names.clear();

            if (!keep_temporary) {
                for (size_t i = 0; i < files.size(); i++) {
                    fs::remove(files[i]);
                }
            }

            t += thr_timer;
        });

    if (!keep_temporary) {
        fs::remove(in_dir);
    }
    if (batch_list.size() == 1) {
        result_file =
            out_dir /
            (batch_list[0].out_file + ClassicIndexHeader::file_extension);
    }

    t.print("classic_combine");
    return (batch_list.size() <= 1);
}

/******************************************************************************/

static inline
uint64_t get_max_file_size(const DocumentList& doc_list,
                           size_t term_size) {
    static constexpr bool debug = false;

    // sort document by file size (as approximation to the number of kmers)
    const std::vector<DocumentEntry>& paths = doc_list.list();
    auto it = std::max_element(
        paths.begin(), paths.end(),
        [](const DocumentEntry& p1, const DocumentEntry& p2) {
            return (std::tie(p1.size_, p1.path_) <
                    std::tie(p2.size_, p2.path_));
        });

    if (it == paths.end())
        return 0;

    // look into largest file and return number of elements
    if (it->type_ == FileType::Text) {
        sLOG << "Max Document Size [Text]:" << it->num_terms(term_size);
        return it->num_terms(term_size);
    }
    else if (it->type_ == FileType::Cortex) {
        sLOG << "Max Document Size [Cortex]:" << it->num_terms(term_size);
        return it->num_terms(term_size);
    }
    else if (it->type_ == FileType::KMerBuffer) {
        sLOG << "Max Document Size [cobs_doc]:" << it->num_terms(term_size);
        return it->num_terms(term_size);
    }
    else if (it->type_ == FileType::Fasta) {
        sLOG << "Max Document Size [Fasta]:" << it->num_terms(term_size);
        return it->num_terms(term_size);
    }
    else if (it->type_ == FileType::FastaMulti) {
        sLOG << "Max Document Size [FastaMulti]:" << it->num_terms(term_size);
        return it->num_terms(term_size);
    }
    else if (it->type_ == FileType::Fastq) {
        sLOG << "Max Document Size [FastQ]:" << it->num_terms(term_size);
        return it->num_terms(term_size);
    }
    die("Unknown file type");
}

void classic_construct(
    const DocumentList& filelist, const fs::path& out_file,
    fs::path tmp_path, ClassicIndexParameters params)
{
    die_unless(params.num_hashes != 0);

    // estimate signature size by finding number of elements in the largest file
    uint64_t max_doc_size = get_max_file_size(filelist, params.term_size);
    if (params.signature_size == 0)
        params.signature_size = calc_signature_size(
            max_doc_size, params.num_hashes, params.false_positive_rate);

    size_t docsize_roundup = tlx::round_up(filelist.size(), 8);

    LOG1 << "Classic Index Parameters:\n"
         << "  term_size: " << params.term_size << '\n'
         << "  canonicalize: " << unsigned(params.canonicalize) << '\n'
         << "  number of documents: " << filelist.size() << '\n'
         << "  maximum document size: " << max_doc_size << '\n'
         << "  num_hashes: " << params.num_hashes << '\n'
         << "  false_positive_rate: " << params.false_positive_rate << '\n'
         << "  signature_size: " << params.signature_size
         << " = " << tlx::format_iec_units(params.signature_size) << '\n'
         << "  row size: " << docsize_roundup / 8 << '\n'
         << "  index size: "
         << tlx::format_iec_units(docsize_roundup / 8 * params.signature_size) << '\n'
         << "  mem_bytes: " << params.mem_bytes
         << " = " << tlx::format_iec_units(params.mem_bytes) << 'B' << '\n'
         << "  clobber: " << unsigned(params.clobber) << '\n'
         << "  continue_: " << unsigned(params.continue_) << '\n'
         << "  keep_temporary: " << unsigned(params.keep_temporary);

    // check output and maybe clobber
    if (!tlx::ends_with(out_file, ClassicIndexHeader::file_extension)) {
        die("Error: classic COBS index file must end with "
            << ClassicIndexHeader::file_extension);
    }

    // check output file
    if (fs::exists(out_file)) {
        if (params.clobber) {
            fs::remove_all(out_file);
        }
        else if (params.continue_) {
            // fall through
        }
        else {
            die("Output file exists, will not overwrite without --clobber");
        }
    }

    // if not set, make tmp path, and maybe clobber
    if (tmp_path.empty()) {
        tmp_path = out_file.string() + ".tmp";
    }
    if (fs::exists(tmp_path)) {
        if (params.clobber) {
            fs::remove_all(tmp_path);
        }
        else if (params.continue_) {
            // fall through
        }
        else {
            die("Temporary directory exists, will not delete without --clobber");
        }
    }

    // create temporary directory
    std::error_code ec;
    fs::create_directories(tmp_path, ec);

    // construct one classic index
    classic_construct_from_documents(filelist, tmp_path / pad_index(1), params);

    // combine batches
    size_t i = 1;
    fs::path result_file;
    while (!classic_combine(
               tmp_path / pad_index(i), tmp_path / pad_index(i + 1),
               result_file,
               params.mem_bytes, params.num_threads, params.keep_temporary)) {
        i++;
    }

    fs::rename(result_file, out_file);

    if (!params.keep_temporary) {
        fs::remove(tmp_path / pad_index(i + 1));
    }

    // cleanup: this will fail if not _all_ temporary files are removed
    if (!params.keep_temporary) {
        fs::remove(tmp_path);
    }
}

void classic_construct_random(const fs::path& out_file,
                              uint64_t signature_size,
                              uint64_t num_documents, size_t document_size,
                              uint64_t num_hashes, size_t seed) {
    Timer t;

    std::vector<std::string> file_names;
    for (size_t i = 0; i < num_documents; ++i) {
        file_names.push_back("file_" + pad_index(i));
    }

    unsigned term_size = 31;
    uint8_t canonicalize = 1;

    ClassicIndexHeader cih;
    cih.term_size_ = term_size;
    cih.canonicalize_ = canonicalize;
    cih.signature_size_ = signature_size;
    cih.num_hashes_ = num_hashes;
    cih.file_names_ = file_names;
    std::vector<uint8_t> data;
    data.resize(signature_size * cih.row_size());

    std::mt19937 rng(seed);
    KMerBuffer<31> doc;

    std::vector<char> canonicalize_buffer(term_size);

    t.active("generate");
    for (size_t i = 0; i < num_documents; ++i) {
        cih.file_names_[i] = file_names[i];
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

        bool shown_canonicalization_warning = false;

        // TODO: parallel for over setting of bits of the SAME document
        for (uint64_t j = 0; j < doc.data().size(); j++) {
            doc.data()[j].canonicalize();
            doc.data()[j].to_string(&term);
            process_term(tlx::string_view(term), data, i, "random",
                         &shown_canonicalization_warning,
                         cih, canonicalize_buffer.data());
        }
    }

    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "ratio of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    cih.write_file(out_file, data);
    t.stop();

    t.print("classic_construct_random");
}

} // namespace cobs

/******************************************************************************/
