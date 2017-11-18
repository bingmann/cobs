#include <CLI/CLI.hpp>
#include <cmath>
#include <util.hpp>
#include <isi/classic_index.hpp>
#include <cortex.hpp>

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

std::experimental::filesystem::path get_path(const std::string& value, const std::string& name, bool needs_to_exist = false) {
    std::experimental::filesystem::path p(value);
    if(needs_to_exist && !std::experimental::filesystem::exists(p)) {
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
    std::experimental::filesystem::path out_file;
    std::experimental::filesystem::path in_dir;
    std::experimental::filesystem::path out_dir;

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
        double signature_size_ratio = isi::calc_signature_size_ratio(p->num_hashes, p->false_positive_rate);
        std::cout << "signature size rate (m / n): " + std::to_string(signature_size_ratio);
        if (p->num_elements != 0) {
            uint64_t signature_size = isi::calc_signature_size(p->num_elements, p->num_hashes, p->false_positive_rate);
            std::cout << std::endl << "signature size (m): " + std::to_string(signature_size);
        }
    });
}

void add_dummy(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("dummy", "creates a dummy classic isi with random content", false);
    p->add_out_file(sub)->required();
    p->add_signature_size(sub)->required();
    p->add_block_size(sub)->required();
    p->add_num_hashes(sub)->required();
    sub->set_callback([p]() {
        isi::classic_index::create_dummy(p->out_file, p->signature_size, p->block_size, p->num_hashes);
    });
}

void add_cortex(CLI::App& app, std::shared_ptr<parameters> p) {
    auto sub = app.add_subcommand("cortex", "converts the cortex files in <in_dir> to the isi sample format", false);
    p->add_in_dir(sub)->required();
    p->add_out_dir(sub)->required();
    sub->set_callback([p]() {
        isi::cortex::process_all_in_directory<31>(p->in_dir, p->out_dir);
    });
}

int main(int argc, char **argv) {
    CLI::App app("(I)nverted (S)ignature (I)ndex for Genome Search\n", false);
    app.require_subcommand(1);
    auto p = std::make_shared<parameters>();
    add_parameters(app, p);
    add_dummy(app, p);
    add_cortex(app, p);
    CLI11_PARSE(app, argc, argv);
    return 0;
}
