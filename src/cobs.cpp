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
#include <CLI/CLI.hpp>
#ifdef __linux__
    #include <cobs/query/compact_index/aio.hpp>
#endif

#include <tlx/cmdline_parser.hpp>
#include <tlx/string.hpp>

#include <random>

/******************************************************************************/
// Cortex File Tools

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

    cp.print_result(std::cerr);

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

template <typename T>
void check_between(std::string name, T num, T min, T max, bool min_inclusive, bool max_inclusive) {
    if ((min_inclusive && num < min) || (!min_inclusive && num <= min) || (max_inclusive && num > max) || (!max_inclusive && num >= max)) {
        std::string interval = (min_inclusive ? "[" : "(") + std::to_string(min) + ", " + std::to_string(max) + (max_inclusive ? "]" : ")");
        throw std::invalid_argument(name + " (" + std::to_string(num) + ") must be in the interval " + interval);
    }
}

uint64_t get_unsigned_integer(const std::string& value, const std::string& name, uint64_t min = 0, uint64_t max = UINT64_MAX, bool min_inclusive = true, bool max_inclusive = true) {
    uint64_t ui;
    double d;
    try {
        d = std::stod(value);
        ui = std::stoull(value);
    } catch (...) {
        throw std::invalid_argument(name + " (" + value + ") is not an unsigned integer");
    }

    if (d < 0) {
        throw std::invalid_argument(name + " (" + value + ") is negative");
    }
    else if (std::floor(d) != d) {
        throw std::invalid_argument(name + " (" + value + ") is not an integer");
    }

    check_between<uint64_t>(name, ui, min, max, min_inclusive, max_inclusive);
    return ui;
}

double get_double(const std::string& value, const std::string& name, double min = 0, double max = UINT64_MAX, bool min_inclusive = true, bool max_inclusive = true) {
    double d;
    try {
        d = std::stod(value);
    } catch (...) {
        throw std::invalid_argument(name + " (" + value + ") is not a double");
    }
    check_between<double>(name, d, min, max, min_inclusive, max_inclusive);
    return d;
}

std::string get_query(std::string& value, const std::string& name) {
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    if (value.find_first_not_of("ACGT") != std::string::npos) {
        throw std::invalid_argument(name + " (" + value + ") does contain characters other than A, C, G and T");
    }
    return value;
}

cobs::fs::path get_path(const std::string& value, const std::string& name, bool needs_to_exist = false) {
    cobs::fs::path p(value);
    if (needs_to_exist && !cobs::fs::exists(p)) {
        throw std::invalid_argument("path " + name + " (" + p.string() + ") does not exist");
    }
    return p;
}

struct parameters {
    uint64_t num_hashes;
    uint64_t signature_size;
    uint64_t block_size;
    double false_positive_rate;
    uint64_t num_elements;
    uint64_t batch_size;
    uint64_t num_result;
    uint64_t page_size;
    uint64_t kmer_size;
    std::string query;
    cobs::fs::path in_file;
    cobs::fs::path out_file;
    cobs::fs::path in_dir;
    cobs::fs::path out_dir;

    CLI::Option * add_num_hashes(CLI::App* app) {
        return app->add_option("<num_hashes>", [&](std::vector<std::string> val) {
                                   this->num_hashes = get_unsigned_integer(val[0], "<num_hashes>", 1);
                                   return true;
                               }, "number of hash functions");
    }

    CLI::Option * add_signature_size(CLI::App* app) {
        return app->add_option("<signature_size>", [&](std::vector<std::string> val) {
                                   this->signature_size = get_unsigned_integer(val[0], "<signature_size>", 1);
                                   return true;
                               }, "number of bits of the signatures (vertical size)");
    }

    CLI::Option * add_block_size(CLI::App* app) {
        return app->add_option("<block_size>", [&](std::vector<std::string> val) {
                                   this->block_size = get_unsigned_integer(val[0], "<block_size>", 1);
                                   return true;
                               }, "horizontal size of the inverted signature index in bytes");
    }

    CLI::Option * add_false_positive_rate(CLI::App* app) {
        return app->add_option("<false_positive_rate>", [&](std::vector<std::string> val) {
                                   this->false_positive_rate = get_double(val[0], "<false_positive_rate>", 0, 1, false, false);
                                   return true;
                               }, "false positive rate");
    }

    CLI::Option * add_num_elements(CLI::App* app) {
        return app->add_option("<num_elements>", [&](std::vector<std::string> val) {
                                   this->num_elements = get_unsigned_integer(val[0], "<num_elements>", 1);
                                   return true;
                               }, "number of elements to be inserted into the index");
    }

    CLI::Option * add_batch_size(CLI::App* app) {
        return app->add_option("<batch_size>", [&](std::vector<std::string> val) {
                                   this->batch_size = get_unsigned_integer(val[0], "<batch_size>", 1);
                                   if (this->batch_size % 8 != 0) {
                                       throw std::invalid_argument("batch_size (" + std::to_string(this->batch_size) + ") must be a multiple of 8");
                                   }
                                   return true;
                               }, "number of input files to be read at once");
    }

    CLI::Option * add_num_result(CLI::App* app) {
        return app->add_option("<num_result>", [&](std::vector<std::string> val) {
                                   this->num_result = get_unsigned_integer(val[0], "<num_result>", 1);
                                   return true;
                               }, "number of results to return");
    }

    CLI::Option * add_page_size(CLI::App* app) {
        return app->add_option("<page_size>", [&](std::vector<std::string> val) {
                                   this->page_size = get_unsigned_integer(val[0], "<page_size>", 1);
                                   return true;
                               }, "the page size of the ssd the index is constructed for");
    }

    CLI::Option * add_kmer_size(CLI::App* app) {
        return app->add_option("<kmer_size>", [&](std::vector<std::string> val) {
                                   this->kmer_size = get_unsigned_integer(val[0], "<kmer_size>", 1);
                                   return true;
                               }, "the size of one kmer");
    }

    CLI::Option * add_query(CLI::App* app) {
        return app->add_option("<query>", [&](std::vector<std::string> val) {
                                   this->query = get_query(val[0], "<query>");
                                   return true;
                               }, "the dna sequence to search for");
    }

    CLI::Option * add_in_file(CLI::App* app) {
        return app->add_option("<in_file>", [&](std::vector<std::string> val) {
                                   this->in_file = get_path(val[0], "<in_file>", true);
                                   return true;
                               }, "path to the input file");
    }

    CLI::Option * add_out_file(CLI::App* app) {
        return app->add_option("<out_file>", [&](std::vector<std::string> val) {
                                   this->out_file = get_path(val[0], "<out_file>");
                                   return true;
                               }, "path to the output file");
    }

    CLI::Option * add_in_dir(CLI::App* app) {
        return app->add_option("<in_dir>", [&](std::vector<std::string> val) {
                                   this->in_dir = get_path(val[0], "<in_dir>", true);
                                   return true;
                               }, "path to the input directory");
    }

    CLI::Option * add_out_dir(CLI::App* app) {
        return app->add_option("<out_dir>", [&](std::vector<std::string> val) {
                                   this->out_dir = get_path(val[0], "<out_dir>");
                                   return true;
                               }, "path to the output directory");
    }
};

void add_parameters(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("parameters", "calculates index parameters", false);
    p->add_num_hashes(sub)->required();
    p->add_false_positive_rate(sub)->required();
    p->add_num_elements(sub);
    sub->set_callback([p]() {
                          if (p->num_elements == 0) {
                              double signature_size_ratio = cobs::calc_signature_size_ratio(p->num_hashes, p->false_positive_rate);
                              std::cout << signature_size_ratio;
                          }
                          else {
                              uint64_t signature_size = cobs::calc_signature_size(p->num_elements, p->num_hashes, p->false_positive_rate);
                              std::cout << signature_size;
                          }
                      });
}

void add_print_sample(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("print_sample", "prints the sample at <in_file>", false);
    p->add_in_file(sub)->required();
    sub->set_callback([p]() {
                          cobs::sample<31> s;
                          cobs::file::sample_header h;
                          cobs::file::deserialize(p->in_file, s, h);
                          std::cout << s;
                      });
}

void add_create_kmers(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("create_kmers", "creates all canonical kmers from <query>", false);
    p->add_query(sub)->required();
    p->add_kmer_size(sub);
    sub->set_callback([p]() {
                          uint64_t kmer_size = p->kmer_size == 0 ? 31U : p->kmer_size;
                          std::vector<char> kmer_raw(kmer_size);
                          for (size_t i = 0; i < p->query.size() - kmer_size; i++) {
                              auto kmer = cobs::query::canonicalize_kmer(p->query.data() + i, kmer_raw.data(), kmer_size);
                              std::cout << std::string(kmer, kmer_size) << "\n";
                          }
                      });
}

void add_cortex(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("cortex", "converts the cortex files in <in_dir> to the cobs sample format", false);
    p->add_in_dir(sub)->required();
    p->add_out_dir(sub)->required();
    sub->set_callback([p]() {
                          cobs::cortex::process_all_in_directory<31>(p->in_dir, p->out_dir);
                      });
}

void add_compact_create_folders(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct_step1", "creates the folders used for further construction", false);
    p->add_in_dir(sub)->required();
    p->add_out_dir(sub)->required();
    p->add_page_size(sub)->required();
    sub->set_callback([p]() {
                          cobs::compact_index::create_folders(p->in_dir, p->out_dir, p->page_size);
                      });
}

void add_compact_combine(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct_combine", "combines the classic indices in <in_dir> to form a compact index", false);
    p->add_in_dir(sub)->required();
    p->add_out_file(sub)->required();
    p->add_page_size(sub)->required();
    sub->set_callback([p]() {
                          cobs::compact_index::combine(p->in_dir, p->out_file, p->page_size);
                      });
}

void add_compact_query(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("query", "queries the index", false);
    p->add_in_file(sub)->required();
    p->add_query(sub)->required();
    p->add_num_result(sub);
    sub->set_callback([p]() {
                          cobs::query::compact_index::mmap mmap(p->in_file);
                          std::vector<std::pair<uint16_t, std::string> > result;
                          mmap.search(p->query, 31, result, p->num_result);
                          for (const auto& res : result) {
                              std::cout << res.second << " - " << res.first << "\n";
                          }
                          std::cout << mmap.get_timer() << std::endl;
                      });
}

#ifdef __linux__
void add_compact_query_aio(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("query_aio", "queries the index", false);
    p->add_in_file(sub)->required();
    p->add_query(sub)->required();
    p->add_num_result(sub);
    sub->set_callback([p]() {
                          cobs::query::compact_index::aio aio(p->in_file);
                          std::vector<std::pair<uint16_t, std::string> > result;
                          aio.search(p->query, 31, result, p->num_result);
                          for (const auto& res : result) {
                              std::cout << res.second << " - " << res.first << "\n";
                          }
                          std::cout << aio.get_timer() << std::endl;
                      });
}
#endif

void add_compact_dummy(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct_dummy", "constructs a dummy index with random content", false);
    sub->set_callback([p]() {
                          throw new std::invalid_argument("not supported yet");
                      });
}

void add_compact(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("compact", "commands for the compact index", false);
    sub->require_subcommand(1);
    add_compact_create_folders(*sub, p);
    add_compact_combine(*sub, p);
    add_compact_query(*sub, p);
#ifdef __linux__
    add_compact_query_aio(*sub, p);
#endif
    add_compact_dummy(*sub, p);
}

int parse(CLI::App& app, int argc, char** argv) {
    try {
        CLI11_PARSE(app, argc, argv);
    } catch (std::ios::failure& e) {
        std::cout << e.what() << " - " << e.code().message() << std::endl;
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
        "cortex_dump", &cortex_dump, false,
        "read a cortex file and dump its samples"
    },
    {
        "classic_construct", &classic_construct, false,
        "constructs a classic index from the samples in <in_dir>"
    },
    {
        "classic_construct_step1", &classic_construct_step1, false,
        "constructs multiple small indices form the samples in <in_dir>"
    },
    {
        "classic_construct_step2", &classic_construct_step2, false,
        "combines the indices in <in_dir>"
    },
    {
        "classic_construct_dummy", &classic_construct_dummy, false,
        "constructs a dummy index with random content"
    },
    {
        "classic_query", &classic_query, false,
        "queries the index"
    },
    { nullptr, nullptr, false, nullptr }
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

    if (argc > 1)
    {
        for (size_t i = 0; subtools[i].name; ++i)
        {
            if (strcmp(subtools[i].name, argv[1]) == 0)
            {
                // replace argv[1] with call string of subtool.
                snprintf(progsub, sizeof(progsub), "%s %s", argv[0], argv[1]);
                argv[1] = progsub;
                std::cerr << "COBS " << subtools[i].name << std::endl;
                return subtools[i].func(argc - 1, argv + 1);
            }
        }
        // std::cout << "Unknown subtool '" << argv[1] << "'" << std::endl;
    }

    // return main_usage(argv[0]);

    CLI::App app("(Co)mpact (B)it-Sliced (S)ignature Index for Genome Search\n", false);
    app.require_subcommand(1);
    auto p = std::make_shared<parameters>();
    add_compact(app, p);
    add_parameters(app, p);
    add_print_sample(app, p);
    add_create_kmers(app, p);
    add_cortex(app, p);
    for (size_t i = 0; subtools[i].name; ++i) {
        app.add_subcommand(subtools[i].name, subtools[i].description, false);
    }
    return parse(app, argc, argv);
}

/******************************************************************************/
