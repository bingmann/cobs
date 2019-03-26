/*******************************************************************************
 * src/cobs.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/compact_index.hpp>
#include <cobs/construction/ranfold_index.hpp>
#include <cobs/cortex_file.hpp>
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/compact_index/mmap.hpp>
#include <cobs/util/parameters.hpp>
#ifdef __linux__
    #include <cobs/query/compact_index/aio.hpp>
#endif

#include <tlx/cmdline_parser.hpp>
#include <tlx/string.hpp>

#include <cmath>
#include <map>
#include <random>

#include <unistd.h>

/******************************************************************************/
// Cortex File Tools

int cortex_convert(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_dir;
    cp.add_param_string(
        "out_dir", out_dir, "path to the output directory");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::process_all_in_directory<31>(in_dir, out_dir);

    return 0;
}

int cortex_dump(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::vector<std::string> filelist;
    cp.add_param_stringlist(
        "filelist", filelist,
        "List of cortex files to process");

    if (!cp.process(argc, argv))
        return -1;

    for (const std::string& file : filelist) {
        cobs::CortexFile cortex(file);

        if (cortex.kmer_size_ == 31u) {
            cortex.process_kmers<31>(
                [&](const cobs::KMer<31>& m) {
                    std::cout << m << '\n';
                });
        }
        else {
            die("cortex_dump: FIXME, add kmer_size " << cortex.kmer_size_);
        }
    }

    return 0;
}

/******************************************************************************/
// "Classical" Index Construction

int classic_construct(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_dir;
    cp.add_param_string(
        "out_dir", out_dir, "path to the output directory");

    unsigned batch_size = 32;
    cp.add_unsigned(
        'b', "batch_size", batch_size,
        "number of input files to be read at once, default: 32");

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num_hashes", num_hashes,
        "number of hash functions, default: 1");

    double false_positive_rate = 0.3;
    cp.add_double(
        'f', "false_positive_rate", false_positive_rate,
        "false positive rate, default: 0.3");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::classic_index::construct(
        in_dir, out_dir, batch_size, num_hashes, false_positive_rate);

    return 0;
}

int classic_construct_step1(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_dir;
    cp.add_param_string(
        "out_dir", out_dir, "path to the output directory");

    unsigned batch_size = 32;
    cp.add_unsigned(
        'b', "batch_size", batch_size,
        "number of input files to be read at once, default: 32");

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num_hashes", num_hashes,
        "number of hash functions, default: 1");

    size_t signature_size = 64 * 1024 * 1024;
    cp.add_size_t(
        's', "signature_size", signature_size,
        "number of bits of the signatures (vertical size), default: 64 M");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    die("FIXME: cobs::classic_index::construct_from_documents");
    // cobs::classic_index::construct_from_documents(
    //     in_dir, out_dir, signature_size, num_hashes, batch_size);

    return 0;
}

int classic_construct_step2(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_dir;
    cp.add_param_string(
        "out_dir", out_dir, "path to the output directory");

    unsigned batch_size = 32;
    cp.add_unsigned(
        'b', "batch_size", batch_size,
        "number of input files to be read at once, default: 32");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::classic_index::combine(in_dir, out_dir, batch_size);

    return 0;
}

int classic_construct_random(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output file");

    size_t signature_size = 2 * 1024 * 1024;
    cp.add_bytes(
        's', "signature_size", signature_size,
        "number of bits of the signatures (vertical size), default: 2 Mi");

    unsigned num_documents = 10000;
    cp.add_unsigned(
        'n', "num_documents", num_documents,
        "number of random documents in index,"
        " default: " + std::to_string(num_documents));

    unsigned document_size = 1000000;
    cp.add_unsigned(
        'm', "document_size", document_size,
        "number of random 31-mers in document,"
        " default: " + std::to_string(document_size));

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num_hashes", num_hashes,
        "number of hash functions, default: 1");

    size_t seed = std::random_device { } ();
    cp.add_size_t("seed", seed, "random seed");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    std::cerr << "Constructing random index"
              << ", num_documents = " << num_documents
              << ", signature_size = " << signature_size
              << ", result size = "
              << tlx::format_iec_units(num_documents / 8 * signature_size)
              << std::endl;

    cobs::classic_index::construct_random(
        out_file, signature_size, num_documents, document_size, num_hashes, seed);

    return 0;
}

int classic_query(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_file;
    cp.add_param_string(
        "in_file", in_file, "path to the input file");

    std::string query;
    cp.add_param_string(
        "query", query, "the dna sequence to search for");

    unsigned num_results = 100;
    cp.add_unsigned(
        'h', "num_results", num_results,
        "number of results to return, default: 100");

    if (!cp.process(argc, argv))
        return -1;

    cobs::query::classic_index::mmap mmap(in_file);
    std::vector<std::pair<uint16_t, std::string> > result;
    mmap.search(query, 31, result, num_results);
    for (const auto& res : result) {
        std::cout << res.second << " - " << res.first << "\n";
    }
    std::cout << mmap.get_timer() << std::endl;

    return 0;
}

/******************************************************************************/
// "Compact" Index Construction

int compact_construct_step1(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_dir;
    cp.add_param_string(
        "out_dir", out_dir, "path to the output directory");

    unsigned page_size = 8192;
    cp.add_unsigned(
        'p', "page_size", page_size,
        "the page size of the SSD the index is constructed for, default: 8192");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    die("FIXME: cobs::compact_index::create_folders");
    // cobs::compact_index::create_folders(in_dir, out_dir, page_size);

    return 0;
}

int compact_construct_combine(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output file");

    unsigned page_size = 8192;
    cp.add_unsigned(
        'p', "page_size", page_size,
        "the page size of the SSD the index is constructed for, default: 8192");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::compact_index::combine(in_dir, out_file, page_size);

    return 0;
}

int compact_query(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_file;
    cp.add_param_string(
        "in_file", in_file, "path to the input file");

    std::string query;
    cp.add_param_string(
        "query", query, "the dna sequence to search for");

    unsigned num_results = 100;
    cp.add_unsigned(
        'h', "num_results", num_results,
        "number of results to return, default: 100");

    bool use_aio = false;
    cp.add_flag('a', "aio", use_aio,
                "queries the index with async I/O operations");

    if (!cp.process(argc, argv))
        return -1;

    if (use_aio) {
#ifdef __linux__
        cobs::query::compact_index::aio aio(in_file);
        std::vector<std::pair<uint16_t, std::string> > result;
        aio.search(query, 31, result, num_results);
        for (const auto& res : result) {
            std::cout << res.second << " - " << res.first << "\n";
        }
        std::cout << aio.get_timer() << std::endl;
#else
        std::cout << "AIO not supported." << std::endl;
        return -1;
#endif
    }
    else {
        cobs::query::compact_index::mmap mmap(in_file);
        std::vector<std::pair<uint16_t, std::string> > result;
        mmap.search(query, 31, result, num_results);
        for (const auto& res : result) {
            std::cout << res.second << " - " << res.first << "\n";
        }
        std::cout << mmap.get_timer() << std::endl;
    }

    return 0;
}

/******************************************************************************/
// "Ranfold" Index Construction

int ranfold_construct(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_dir;
    cp.add_param_string(
        "in_dir", in_dir, "path to the input directory");

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output file");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    cobs::ranfold_index::construct(in_dir, out_file);

    return 0;
}

int ranfold_construct_random(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output file");

    size_t term_space = 2 * 1024 * 1024;
    cp.add_bytes(
        't', "term_space", term_space,
        "size of term space for kmer signatures (vertical size), "
        "default: " + tlx::format_iec_units(term_space));

    unsigned num_term_hashes = 1;
    cp.add_unsigned(
        'T', "term_hashes", num_term_hashes,
        "number of hash functions per term, default: 1");

    size_t doc_space_bits = 16 * 1024;
    cp.add_bytes(
        'd', "doc_space_bits", doc_space_bits,
        "number of bits in the document space (horizontal size), "
        "default: " + tlx::format_iec_units(doc_space_bits));

    unsigned num_doc_hashes = 2;
    cp.add_unsigned(
        'D', "doc_hashes", num_doc_hashes,
        "number of hash functions per term, default: 2");

    unsigned num_documents = 10000;
    cp.add_unsigned(
        'n', "num_documents", num_documents,
        "number of random documents in index,"
        " default: " + std::to_string(num_documents));

    unsigned document_size = 1000000;
    cp.add_unsigned(
        'm', "document_size", document_size,
        "number of random 31-mers in document,"
        " default: " + std::to_string(document_size));

    size_t seed = std::random_device { } ();
    cp.add_size_t("seed", seed, "random seed");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    std::cerr << "Constructing ranfold index"
              << ", term_space = " << term_space
              << ", num_term_hashes = " << num_term_hashes
              << ", doc_space_bits = " << doc_space_bits
              << ", num_doc_hashes = " << num_doc_hashes
              << ", num_documents = " << num_documents
              << ", document_size = " << document_size
              << ", result size = "
              << tlx::format_iec_units(
        (term_space * (doc_space_bits + 7) / 8))
              << std::endl;

    cobs::ranfold_index::construct_random(
        out_file,
        term_space, num_term_hashes,
        doc_space_bits, num_doc_hashes,
        num_documents, document_size,
        seed);

    return 0;
}

/******************************************************************************/
// Miscellaneous Methods

int print_parameters(int argc, char** argv) {
    tlx::CmdlineParser cp;

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num_hashes", num_hashes,
        "number of hash functions, default: 1");

    double false_positive_rate = 0.3;
    cp.add_double(
        'f', "false_positive_rate", false_positive_rate,
        "false positive rate, default: 0.3");

    unsigned num_elements = 0;
    cp.add_unsigned(
        'n', "num_elements", num_elements,
        "number of elements to be inserted into the index");

    if (!cp.process(argc, argv))
        return -1;

    if (num_elements == 0) {
        double signature_size_ratio =
            cobs::calc_signature_size_ratio(num_hashes, false_positive_rate);
        std::cout << signature_size_ratio << '\n';
    }
    else {
        uint64_t signature_size =
            cobs::calc_signature_size(num_elements, num_hashes, false_positive_rate);
        std::cout << signature_size << '\n';
    }

    return 0;
}

int print_basepair_map(int, char**) {
    char basepair_map[256];
    memset(basepair_map, 0, sizeof(basepair_map));
    basepair_map['A'] = 'T';
    basepair_map['C'] = 'G';
    basepair_map['G'] = 'C';
    basepair_map['T'] = 'A';
    for (size_t i = 0; i < sizeof(basepair_map); ++i) {
        std::cout << int(basepair_map[i]) << ',';
        if (i % 16 == 15) {
            std::cout << '\n';
        }
    }
    return 0;
}

int print_kmers(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string query;
    cp.add_param_string(
        "query", query, "the dna sequence to search for");

    unsigned kmer_size = 31;
    cp.add_unsigned(
        'k', "kmer_size", kmer_size,
        "the size of one kmer, default: 31");

    if (!cp.process(argc, argv))
        return -1;

    char kmer_buffer[kmer_size];
    for (size_t i = 0; i < query.size() - kmer_size; i++) {
        auto kmer = cobs::canonicalize_kmer(
            query.data() + i, kmer_buffer, kmer_size);
        std::cout << std::string(kmer, kmer_size) << '\n';
    }

    return 0;
}

/******************************************************************************/
// Benchmarking

template <bool FalsePositiveDist>
void benchmark_fpr_run(const cobs::fs::path& p,
                       const std::vector<std::string>& queries,
                       const std::vector<std::string>& warmup_queries) {

    cobs::query::classic_index::mmap s(p);

    sync();
    std::ofstream ofs("/proc/sys/vm/drop_caches");
    ofs << "3" << std::endl;
    ofs.close();

    std::vector<std::pair<uint16_t, std::string> > result;
    for (size_t i = 0; i < warmup_queries.size(); i++) {
        s.search(warmup_queries[i], 31, result);
    }
    s.get_timer().reset();

    std::map<uint32_t, uint64_t> counts;

    for (size_t i = 0; i < queries.size(); i++) {
        s.search(queries[i], 31, result);

        if (FalsePositiveDist) {
            for (const auto& r : result) {
                counts[r.first]++;
            }
        }
    }

    std::string simd = "on";
    std::string openmp = "on";
    std::string aio = "on";

#ifdef NO_SIMD
    simd = "off";
#endif

#ifdef NO_OPENMP
    openmp = "off";
#endif

#ifdef NO_AIO
    aio = "off";
#endif

    cobs::Timer t = s.get_timer();
    std::cout << "RESULT"
              << " name=benchmark "
              << " index=" << p.string()
              << " kmer_queries=" << (queries[0].size() - 30)
              << " queries=" << queries.size()
              << " warmup=" << warmup_queries.size()
              << " results=" << result.size()
              << " simd=" << simd
              << " openmp=" << openmp
              << " aio=" << aio
              << " t_hashes=" << t.get("hashes")
              << " t_io=" << t.get("io")
              << " t_and=" << t.get("and rows")
              << " t_add=" << t.get("add rows")
              << " t_sort=" << t.get("sort results")
              << std::endl;

    for (const auto& c : counts) {
        std::cout << "RESULT"
                  << " name=benchmark_fpr"
                  << " fpr=" << c.first
                  << " dist=" << c.second
                  << std::endl;
    }
}

int benchmark_fpr(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_file;
    cp.add_param_string(
        "in_file", in_file, "path to the input file");

    unsigned num_kmers = 1000;
    cp.add_unsigned(
        'k', "num_kmers", num_kmers,
        "number of kmers of each query, "
        "default: " + std::to_string(num_kmers));

    unsigned num_queries = 10000;
    cp.add_unsigned(
        'q', "queries", num_queries,
        "number of random queries to run, "
        "default: " + std::to_string(num_queries));

    unsigned num_warmup = 100;
    cp.add_unsigned(
        'w', "warmup", num_warmup,
        "number of random warmup queries to run, "
        "default: " + std::to_string(num_warmup));

    bool fpr_dist = false;
    cp.add_flag(
        'd', "dist", fpr_dist,
        "calculate false positive distribution");

    size_t seed = std::random_device { } ();
    cp.add_size_t("seed", seed, "random seed");

    if (!cp.process(argc, argv))
        return -1;

    std::mt19937 rng(seed);

    std::vector<std::string> warmup_queries;
    std::vector<std::string> queries;
    for (size_t i = 0; i < num_warmup; i++) {
        warmup_queries.push_back(
            cobs::random_sequence_rng(num_kmers + 30, rng));
    }
    for (size_t i = 0; i < num_queries; i++) {
        queries.push_back(
            cobs::random_sequence_rng(num_kmers + 30, rng));
    }

    if (fpr_dist)
        benchmark_fpr_run</* FalsePositiveDist */ true>(
            in_file, queries, warmup_queries);
    else
        benchmark_fpr_run</* FalsePositiveDist */ false>(
            in_file, queries, warmup_queries);

    return 0;
}

/******************************************************************************/

struct SubTool {
    const char* name;
    int (* func)(int argc, char* argv[]);
    bool shortline;
    const char* description;
};

struct SubTool subtools[] = {
    {
        "cortex_convert", &cortex_convert, true,
        "converts the cortex files in <in_dir> to the cobs document format"
    },
    {
        "cortex_dump", &cortex_dump, true,
        "read a cortex file and dump its documents"
    },
    {
        "classic_construct", &classic_construct, true,
        "constructs a classic index from the documents in <in_dir>"
    },
    {
        "classic_construct_step1", &classic_construct_step1, true,
        "constructs multiple small indices from the documents in <in_dir>"
    },
    {
        "classic_construct_step2", &classic_construct_step2, true,
        "combines the indices in <in_dir>"
    },
    {
        "classic_construct_random", &classic_construct_random, true,
        "constructs an index with random content"
    },
    {
        "classic_query", &classic_query, true,
        "queries the index"
    },
    {
        "compact_construct_step1", &compact_construct_step1, true,
        "creates the folders used for further construction"
    },
    {
        "compact_construct_combine", &compact_construct_combine, true,
        "combines the classic indices in <in_dir> to form a compact index"
    },
    {
        "compact_query", &compact_query, true,
        "queries the index"
    },
    {
        "ranfold_construct", &ranfold_construct, true,
        "constructs a ranfold index from documents"
    },
    {
        "ranfold_construct_random", &ranfold_construct_random, true,
        "constructs a ranfold index with random content"
    },
    {
        "print_parameters", &print_parameters, true,
        "calculates index parameters"
    },
    {
        "print_kmers", &print_kmers, true,
        "print all canonical kmers from <query>"
    },
    {
        "print_basepair_map", &print_basepair_map, true,
        "print canonical basepair character mapping"
    },
    {
        "benchmark_fpr", &benchmark_fpr, true,
        "run benchmark and false positive measurement"
    },
    { nullptr, nullptr, true, nullptr }
};

int main_usage(const char* arg0) {
    std::cout << "(Co)mpact (B)it-Sliced (S)ignature Index for Genome Search"
              << std::endl << std::endl;
    std::cout << "Usage: " << arg0 << " <subtool> ..." << std::endl << std::endl
              << "Available subtools: " << std::endl;

    int shortlen = 0;

    for (size_t i = 0; subtools[i].name; ++i)
    {
        if (!subtools[i].shortline) continue;
        shortlen = std::max(shortlen, static_cast<int>(strlen(subtools[i].name)));
    }

    for (size_t i = 0; subtools[i].name; ++i)
    {
        if (subtools[i].shortline) continue;
        std::cout << "  " << subtools[i].name << std::endl;
        tlx::CmdlineParser::output_wrap(
            std::cout, subtools[i].description, 80, 6, 6);
        std::cout << std::endl;
    }

    for (size_t i = 0; subtools[i].name; ++i)
    {
        if (!subtools[i].shortline) continue;
        std::cout << "  " << std::left << std::setw(shortlen + 2)
                  << subtools[i].name << subtools[i].description << std::endl;
    }
    std::cout << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    char progsub[256];

    if (argc < 2)
        return main_usage(argv[0]);

    for (size_t i = 0; subtools[i].name; ++i)
    {
        if (strcmp(subtools[i].name, argv[1]) == 0)
        {
            // replace argv[1] with call string of subtool.
            snprintf(progsub, sizeof(progsub), "%s %s", argv[0], argv[1]);
            argv[1] = progsub;
            try {
                return subtools[i].func(argc - 1, argv + 1);
            }
            catch (std::exception& e) {
                std::cerr << "EXCEPTION: " << e.what() << std::endl;
                return -1;
            }
        }
    }

    std::cout << "Unknown subtool '" << argv[1] << "'" << std::endl;

    return main_usage(argv[0]);
}

/******************************************************************************/
