#include <iostream>
#include <map>
#include <cortex.hpp>
#include <bit_sliced_signatures/bss.hpp>
#include <type_traits>
#include <docopt.h>

static const char USAGE[] =
        R"((I)nverted (S)ignature (I)ndex for Genome Search.

    Usage:
        isi cortex <in_dir> <out_dir>
        isi bss <in_dir> <out_dir> <signature_size> <block_size> <num_hashes>
        isi dummy <path> <signature_size> <block_size> <num_hashes>

    Options:
        -h --help     Show this screen.
        -v --version  Show version.
)";

typedef std::map<std::string, docopt::value> cli_map;

template<typename T>
struct value_funcs {
    std::function<bool(docopt::value const&)> is;
    std::function<T(docopt::value const&)> as;
};

template<typename T>
value_funcs<T> get_value_funcs();

template<>
value_funcs<std::string> get_value_funcs() {
    return {&docopt::value::isString, &docopt::value::asString};
}

template<>
value_funcs<bool> get_value_funcs() {
    return {&docopt::value::isBool, &docopt::value::asBool};
}

template<>
value_funcs<long> get_value_funcs() {
    return {&docopt::value::isLong, &docopt::value::asLong};
}

template<typename T>
T get_value(cli_map& map, const std::string& name) {
    static_assert(std::is_same<T, std::string>::value || std::is_same<T, bool>::value || std::is_same<T, long>::value);
    value_funcs<T> vs = get_value_funcs<T>();

    if (!vs.is(map[name])) {
        throw std::invalid_argument(name + " is not a " + typeid(T).name());
    }
    return vs.as(map[name]);
}

std::experimental::filesystem::path get_path(cli_map& map, const std::string& name, bool needs_to_exist) {
    std::string s = get_value<std::string>(map, name);
    std::experimental::filesystem::path p(s);
    if(needs_to_exist && !std::experimental::filesystem::exists(p)) {
        throw std::invalid_argument("path " + name + " (" + p.string() + ") does not exist");
    }
    return p;
}

uint64_t get_unsigned_integer(cli_map& map, const std::string& name) {
    long l = get_value<long>(map, name);
    if(l > 0) {
        throw std::invalid_argument("unsigned integer " + name + " (" + std::to_string(l) + ") is negative");
    }
    return (uint64_t) l;
}

void cortex(cli_map& map) {
    auto in_dir = get_path(map, "<in_dir>", true);
    auto out_dir = get_path(map, "<out_dir>", false);
    isi::cortex::process_all_in_directory<31>(in_dir, out_dir);
}

void bss(cli_map& map) {
    auto in_dir = get_path(map, "<in_dir>", true);
    auto out_dir = get_path(map, "<out_dir>", false);
    uint64_t signature_size = get_unsigned_integer(map, "<signature_size>");
    uint64_t block_size = get_unsigned_integer(map, "<block_size>");
    uint64_t num_hashes = get_unsigned_integer(map, "<num_hashes");
    isi::bss::create_from_samples(in_dir, out_dir, signature_size, block_size, num_hashes);
}

void dummy(cli_map& map) {
    auto path = get_path(map, "<path>", false);
    uint64_t signature_size = get_unsigned_integer(map, "<signature_size>");
    uint64_t block_size = get_unsigned_integer(map, "<block_size>");
    uint64_t num_hashes = get_unsigned_integer(map, "<num_hashes");
    isi::bss::generate_dummy(path, signature_size, block_size, num_hashes);
}

int main(int argc, const char** argv) {
    cli_map map = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "1.0.0");

    std::unordered_map<std::string, std::function<void(cli_map&)>> callbacks = {
            {"cortex", cortex},
            {"bss", bss},
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
