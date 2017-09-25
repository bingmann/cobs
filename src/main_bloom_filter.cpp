#include <boost/filesystem/path.hpp>
#include <bloom_filter.hpp>

int main(int argc, char** argv) {
    std::string in_dir = "/users/flo/projects/thesis/data/bin";
    std::string out_dir = "/users/flo/projects/thesis/data/bloom";
    size_t bloom_filter_size = 200000;
    size_t block_size = 1;
    size_t num_hashes = 7;

    if (argc == 6) {
        char* pEnd;
        in_dir = argv[1];
        out_dir = argv[2];
        bloom_filter_size = std::strtoul(argv[3], &pEnd, 10);
        block_size = std::strtoul(argv[4], &pEnd, 10);
        num_hashes = std::strtoul(argv[5], &pEnd, 10);
    } else {
        std::cout << "wrong number of args: in_dir out_dir bloom_filter_size block_size num_hashes" << std::endl;
    }

    genome::bloom_filter::process_all_in_directory(in_dir, out_dir, bloom_filter_size, block_size, num_hashes);

    int a = 0;
}

