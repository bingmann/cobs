/*******************************************************************************
 * src/cobs.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cmath>
#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/compact_index.hpp>
#include <cobs/cortex.hpp>
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/compact_index/mmap.hpp>
#include <cobs/util/parameters.hpp>
#ifdef __linux__
    #include <cobs/query/compact_index/aio.hpp>
#endif

#include <tlx/cmdline_parser.hpp>
#include <tlx/string.hpp>

#include <random>

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

    cobs::cortex::process_all_in_directory<31>(in_dir, out_dir);

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
        cobs::cortex::CortexFile cortex(file);

        if (cortex.kmer_size_ == 31u) {
            cortex.process_kmers<31>(
                [&](const cobs::kmer<31>& m) {
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

    cobs::classic_index::create_from_samples(
        in_dir, out_dir, signature_size, num_hashes, batch_size);

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

int classic_construct_dummy(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string out_file;
    cp.add_param_string(
        "out_file", out_file, "path to the output file");

    size_t signature_size = 8 * 1024 * 1024;
    cp.add_size_t(
        's', "signature_size", signature_size,
        "number of bits of the signatures (vertical size), default: 8 Mi");

    unsigned block_size = 1024;
    cp.add_unsigned(
        'b', "block_size", block_size,
        "horizontal size of the inverted signature index in bytes, default: 1024");

    unsigned num_hashes = 1;
    cp.add_unsigned(
        'h', "num_hashes", num_hashes,
        "number of hash functions, default: 1");

    size_t seed = std::random_device { } ();
    cp.add_size_t("seed", seed, "random seed");

    if (!cp.process(argc, argv))
        return -1;

    cp.print_result(std::cerr);

    std::cerr << "Constructing dummy index, num_documents = " << 8 * block_size
              << ", result size = "
              << tlx::format_iec_units(block_size * signature_size)
              << std::endl;

    cobs::classic_index::create_dummy(
        out_file, signature_size, block_size, num_hashes, seed);

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

    cobs::compact_index::create_folders(in_dir, out_dir, page_size);

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

    std::vector<char> kmer_raw(kmer_size);
    for (size_t i = 0; i < query.size() - kmer_size; i++) {
        auto kmer = cobs::query::canonicalize_kmer(
            query.data() + i, kmer_raw.data(), kmer_size);
        std::cout << std::string(kmer, kmer_size) << '\n';
    }

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
        "converts the cortex files in <in_dir> to the cobs sample format"
    },
    {
        "cortex_dump", &cortex_dump, true,
        "read a cortex file and dump its samples"
    },
    {
        "classic_construct", &classic_construct, true,
        "constructs a classic index from the samples in <in_dir>"
    },
    {
        "classic_construct_step1", &classic_construct_step1, true,
        "constructs multiple small indices form the samples in <in_dir>"
    },
    {
        "classic_construct_step2", &classic_construct_step2, true,
        "combines the indices in <in_dir>"
    },
    {
        "classic_construct_dummy", &classic_construct_dummy, true,
        "constructs a dummy index with random content"
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
