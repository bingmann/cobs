#include <experimental/filesystem>
#include <bit_sliced_signatures/bss.hpp>

int main(int argc, char** argv) {
    if (argc == 6) {
        char* pEnd;
        std::string in_dir = argv[1];
        std::string out_dir = argv[2];
        size_t signature_size = std::strtoul(argv[3], &pEnd, 10);
        size_t block_size = std::strtoul(argv[4], &pEnd, 10);
        size_t num_hashes = std::strtoul(argv[5], &pEnd, 10);
        genome::bss::create_from_samples(in_dir, out_dir, signature_size, block_size, num_hashes);
    } else {
        std::cout << "wrong number of args: in_dir out_dir signature_size block_size num_hashes" << std::endl;
    }
}

