#include <iostream>
#include <map>
#include <cortex.hpp>
#include <isi/classic_index.hpp>
#include <type_traits>
#include <docopt.h>

static const char USAGE[] =
        R"(================================================
(I)nverted (S)ignature (I)ndex for Genome Search
================================================

    usage:
        isi cortex <in_dir> <out_dir>
        isi classic_index <in_dir> <out_dir> <signature_size> <block_size> <num_hashes>
        isi dummy <path> <signature_size> <block_size> <num_hashes>
        isi parameters <num_hashes> <false_positive_rate> [<num_elements>]

    options:
        -h --help     Show this screen.
        -v --version  Show version.

    description:
        isi cortex        # converts the cortex files in <in_dir> to the isi sample format
        isi dummy         # creates a dummy classic isi with random content
        isi parameters    # calculates bloom filter parameters
)";

typedef std::map<std::string, docopt::value> cli_map;

template<typename T>
std::function<T(docopt::value const&)> get_accessor_func();

template<>
std::function<std::string(docopt::value const&)> get_accessor_func() {
    return &docopt::value::asString;
}

template<>
std::function<bool(docopt::value const&)> get_accessor_func() {
    return &docopt::value::asBool;
}

template<>
std::function<long(docopt::value const&)> get_accessor_func() {
    return &docopt::value::asLong;
}

template<typename T>
T get_value(cli_map& map, const std::string& name) {
    static_assert(std::is_same<T, std::string>::value || std::is_same<T, bool>::value || std::is_same<T, long>::value);
    std::function<T(docopt::value const&)> accessor_func = get_accessor_func<T>();

    try {
        return accessor_func(map[name]);
    } catch (...) {
        throw std::invalid_argument(name + " is not a " + typeid(T).name());
    }
}

template<typename T>
void check_between(std::string name, T num, double min, double max, bool min_inclusive, bool max_inclusive) {
    if ((min_inclusive && num < min) || (!min_inclusive && num <= min) || (max_inclusive && num > max) || (!max_inclusive && num >= max)) {
        std::string interval = (min_inclusive ? "[" : "(") + std::to_string(min) + ", " + std::to_string(max) + (max_inclusive ? "]" : ")");
        throw std::invalid_argument(name + " (" + std::to_string(num) + ") must be in the interval " + interval);
    }
}

std::experimental::filesystem::path get_path(cli_map& map, const std::string& name, bool needs_to_exist) {
    std::string s = get_value<std::string>(map, name);
    std::experimental::filesystem::path p(s);
    if(needs_to_exist && !std::experimental::filesystem::exists(p)) {
        throw std::invalid_argument("path " + name + " (" + p.string() + ") does not exist");
    }
    return p;
}

uint64_t get_unsigned_integer(cli_map& map, const std::string& name, uint64_t min = 0, uint64_t max = UINT64_MAX, bool min_inclusive = true, bool max_inclusive = true) {
    long l = get_value<long>(map, name);
    check_between(name, l, min, max, min_inclusive, max_inclusive);
    return (uint64_t) l;
}

double get_double(cli_map& map, const std::string& name) {
    std::string d = get_value<std::string>(map, name);
    try {
        return std::stod(d);
    } catch (...) {
        throw std::invalid_argument("double " + name + " (" + d + ") is not a double");
    }
}

void cortex(cli_map& map) {
    auto in_dir = get_path(map, "<in_dir>", true);
    auto out_dir = get_path(map, "<out_dir>", false);
    isi::cortex::process_all_in_directory<31>(in_dir, out_dir);
}

void classic_index(cli_map& map) {
    auto in_dir = get_path(map, "<in_dir>", true);
    auto out_dir = get_path(map, "<out_dir>", false);
    uint64_t signature_size = get_unsigned_integer(map, "<signature_size>", 1);
    uint64_t block_size = get_unsigned_integer(map, "<block_size>", 1);
    uint64_t num_hashes = get_unsigned_integer(map, "<num_hashes", 1);
    isi::classic_index::create_from_samples(in_dir, out_dir, signature_size, block_size, num_hashes);
}

void parameters(cli_map& map) {
    size_t num_hashes = get_unsigned_integer(map, "<num_hashes>", 1);
    double false_positive_rate = get_double(map, "<false_positive_rate>");
    check_between("<false_positive_rate>", false_positive_rate, 0, 1, false, false);

    double signature_size_ratio = isi::calc_signature_size_ratio(num_hashes, false_positive_rate);
    std::cout << "signature size rate (m / n): " + std::to_string(signature_size_ratio);
    if (map["<num_elements>"]) {
        size_t num_elements = get_unsigned_integer(map, "<num_elements>", 1);
        uint64_t signature_size = isi::calc_signature_size(num_elements, num_hashes, false_positive_rate);
        std::cout << std::endl << "signature size (m): " + std::to_string(signature_size);
    }
}

void dummy(cli_map& map) {
    auto path = get_path(map, "<path>", false);
    uint64_t signature_size = get_unsigned_integer(map, "<signature_size>", 1);
    uint64_t block_size = get_unsigned_integer(map, "<block_size>", 1);
    uint64_t num_hashes = get_unsigned_integer(map, "<num_hashes", 1);
    isi::classic_index::generate_dummy(path, signature_size, block_size, num_hashes);
}

int main(int argc, const char** argv) {
    cli_map map = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "1.0.0");

    std::unordered_map<std::string, std::function<void(cli_map&)>> callbacks = {
            {"cortex", cortex},
            {"classic_index", classic_index},
            {"parameters", parameters},
            {"dummy", dummy},
    };

    try {
        for (const auto& cb: callbacks) {
            if (map[cb.first].asBool()) {
                cb.second(map);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
