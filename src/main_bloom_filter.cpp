#include <boost/filesystem/path.hpp>
#include <bloom_filter.hpp>

int main(int argc, char** argv) {
    if (argc == 6) {
        char* pEnd;
        std::string in_dir = argv[1];
        std::string out_dir = argv[2];
        size_t bloom_filter_size = std::strtoul(argv[3], &pEnd, 10);
        size_t block_size = std::strtoul(argv[4], &pEnd, 10);
        size_t num_hashes = std::strtoul(argv[5], &pEnd, 10);
        genome::bloom_filter::create_from_samples(in_dir, out_dir, bloom_filter_size, block_size, num_hashes);
    } else {
        std::cout << "wrong number of args: in_dir out_dir bloom_filter_size block_size num_hashes" << std::endl;
    }
}

