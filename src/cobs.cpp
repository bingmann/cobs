#include <CLI/CLI.hpp>
#include <cmath>
#include <cobs/construction/classic_index.hpp>
#include <cobs/cortex.hpp>
#include <cobs/util/parameters.hpp>
#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/compact_index/mmap.hpp>
#include <cobs/construction/compact_index.hpp>
#ifdef __linux__
    #include <cobs/query/compact_index/aio.hpp>
#endif

template<typename T>
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
    } else if (std::floor(d) != d) {
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
    if(needs_to_exist && !cobs::fs::exists(p)) {
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

    CLI::Option* add_num_hashes(CLI::App* app) {
        return app->add_option("<num_hashes>", [&](std::vector<std::string> val) {
            this->num_hashes = get_unsigned_integer(val[0], "<num_hashes>", 1);
            return true;
        }, "number of hash functions");
    }

    CLI::Option* add_signature_size(CLI::App* app) {
        return app->add_option("<signature_size>", [&](std::vector<std::string> val) {
            this->signature_size = get_unsigned_integer(val[0], "<signature_size>", 1);
            return true;
        }, "number of bits of the signatures (vertical size)");
    }

    CLI::Option* add_block_size(CLI::App* app) {
        return app->add_option("<block_size>", [&](std::vector<std::string> val) {
            this->block_size = get_unsigned_integer(val[0], "<block_size>", 1);
            return true;
        }, "horizontal size of the inverted signature index in bytes");
    }

    CLI::Option* add_false_positive_rate(CLI::App* app) {
        return app->add_option("<false_positive_rate>", [&](std::vector<std::string> val) {
            this->false_positive_rate = get_double(val[0], "<false_positive_rate>", 0, 1, false, false);
            return true;
        }, "false positive rate");
    }

    CLI::Option* add_num_elements(CLI::App* app) {
        return app->add_option("<num_elements>", [&](std::vector<std::string> val) {
            this->num_elements = get_unsigned_integer(val[0], "<num_elements>", 1);
            return true;
        }, "number of elements to be inserted into the index");
    }

    CLI::Option* add_batch_size(CLI::App* app) {
        return app->add_option("<batch_size>", [&](std::vector<std::string> val) {
            this->batch_size = get_unsigned_integer(val[0], "<batch_size>", 1);
            if(this->batch_size % 8 != 0) {
                throw std::invalid_argument("batch_size (" + std::to_string(this->batch_size) + ") must be a multiple of 8");
            }
            return true;
        }, "number of input files to be read at once");
    }

    CLI::Option* add_num_result(CLI::App* app) {
        return app->add_option("<num_result>", [&](std::vector<std::string> val) {
            this->num_result = get_unsigned_integer(val[0], "<num_result>", 1);
            return true;
        }, "number of results to return");
    }

    CLI::Option* add_page_size(CLI::App* app) {
        return app->add_option("<page_size>", [&](std::vector<std::string> val) {
            this->page_size = get_unsigned_integer(val[0], "<page_size>", 1);
            return true;
        }, "the page size of the ssd the index is constructed for");
    }

    CLI::Option* add_kmer_size(CLI::App* app) {
        return app->add_option("<kmer_size>", [&](std::vector<std::string> val) {
            this->kmer_size = get_unsigned_integer(val[0], "<kmer_size>", 1);
            return true;
        }, "the size of one kmer");
    }

    CLI::Option* add_query(CLI::App* app) {
        return app->add_option("<query>", [&](std::vector<std::string> val) {
            this->query = get_query(val[0], "<query>");
            return true;
        }, "the dna sequence to search for");
    }

    CLI::Option* add_in_file(CLI::App* app) {
        return app->add_option("<in_file>", [&](std::vector<std::string> val) {
            this->in_file = get_path(val[0], "<in_file>", true);
            return true;
        }, "path to the input file");
    }

    CLI::Option* add_out_file(CLI::App* app) {
        return app->add_option("<out_file>", [&](std::vector<std::string> val) {
            this->out_file = get_path(val[0], "<out_file>");
            return true;
        }, "path to the output file");
    }

    CLI::Option* add_in_dir(CLI::App* app) {
        return app->add_option("<in_dir>", [&](std::vector<std::string> val) {
            this->in_dir = get_path(val[0], "<in_dir>", true);
            return true;
        }, "path to the input directory");
    }

    CLI::Option* add_out_dir(CLI::App* app) {
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
        } else {
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

void add_print_header(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("print_header", "prints the header at <in_file>", false);
    p->add_in_file(sub)->required();
    sub->set_callback([p]() {
        throw new std::invalid_argument("not supported yet");
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

void add_classic_create(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct", "constructs an index from the samples in <in_dir>", false);
    p->add_in_dir(sub)->required();
    p->add_out_dir(sub)->required();
    p->add_batch_size(sub)->required();
    p->add_num_hashes(sub)->required();
    p->add_false_positive_rate(sub)->required();
    sub->set_callback([p]() {
        cobs::classic_index::create(p->in_dir, p->out_dir, p->batch_size, p->num_hashes, p->false_positive_rate);
    });
}

void add_classic_create_from_samples(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct_step1", "constructs multiple small indices form the samples in <in_dir>", false);
    p->add_in_dir(sub)->required();
    p->add_out_dir(sub)->required();
    p->add_signature_size(sub)->required();
    p->add_num_hashes(sub)->required();
    p->add_batch_size(sub)->required();
    sub->set_callback([p]() {
        cobs::classic_index::create_from_samples(p->in_dir, p->out_dir, p->signature_size, p->num_hashes, p->batch_size);
    });
}

void add_classic_combine(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct_step2", "combines the indices in <in_dir>", false);
    p->add_in_dir(sub)->required();
    p->add_out_dir(sub)->required();
    p->add_batch_size(sub)->required();
    sub->set_callback([p]() {
        cobs::classic_index::combine(p->in_dir, p->out_dir, p->batch_size);
    });
}

void add_classic_query(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("query", "queries the index", false);
    p->add_in_file(sub)->required();
    p->add_query(sub)->required();
    p->add_num_result(sub);
    sub->set_callback([p]() {
        cobs::query::classic_index::mmap mmap(p->in_file);
        std::vector<std::pair<uint16_t, std::string>> result;
        mmap.search(p->query, 31, result, p->num_result);
        for (const auto& res: result) {
            std::cout << res.second << " - " << res.first << "\n";
        }
        std::cout << mmap.get_timer() << std::endl;
    });
}

void add_classic_dummy(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("construct_dummy", "constructs a dummy index with random content", false);
    p->add_out_file(sub)->required();
    p->add_signature_size(sub)->required();
    p->add_block_size(sub)->required();
    p->add_num_hashes(sub)->required();
    sub->set_callback([p]() {
        cobs::classic_index::create_dummy(p->out_file, p->signature_size, p->block_size, p->num_hashes);
    });
}

void add_classic(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("classic", "commands for the classic index", false);
    sub->require_subcommand(1);
    add_classic_create(*sub, p);
    add_classic_create_from_samples(*sub, p);
    add_classic_combine(*sub, p);
    add_classic_query(*sub, p);
    add_classic_dummy(*sub, p);
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
        std::vector<std::pair<uint16_t, std::string>> result;
        mmap.search(p->query, 31, result, p->num_result);
        for (const auto& res: result) {
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
        std::vector<std::pair<uint16_t, std::string>> result;
        aio.search(p->query, 31, result, p->num_result);
        for (const auto& res: result) {
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

int parse(CLI::App& app, int argc, char **argv) {
    try {
        CLI11_PARSE(app, argc, argv);
    } catch (std::ios::failure e) {
        std::cout << e.what() << " - " << e.code().message() << std::endl;
    }
    return 0;
}

int main(int argc, char **argv) {
    CLI::App app("(I)nverted (S)ignature (I)ndex for Genome Search\n", false);
    app.require_subcommand(1);
    auto p = std::make_shared<parameters>();
    add_classic(app, p);
    add_compact(app, p);
    add_parameters(app, p);
    add_print_sample(app, p);
    add_print_header(app, p);
    add_create_kmers(app, p);
    add_cortex(app, p);
    return parse(app, argc, argv);
}
