#include <file/bss_header.hpp>
#include <file/util.hpp>

void generate_dummy_bloom(boost::filesystem::path p, size_t signature_size, size_t block_size, size_t num_hashes) {
    std::vector<std::string> file_names;
    for(size_t i = 0; i < 8 * block_size; i++) {
        file_names.push_back("file_" + std::to_string(i));
    }
    genome::file::bss_header bssh(signature_size, block_size, num_hashes, file_names);
    std::ofstream ofs;
    genome::file::serialize_header(ofs, p, bssh);

    for (size_t i = 0; i < signature_size * block_size; i += 4) {
        int rnd = std::rand();
        ofs.write(reinterpret_cast<char*>(&rnd), 4);
    }
}

int main(int argc, char** argv) {
    if (argc == 5) {
        char* pEnd;
        std::string path = argv[1];
        size_t signature_size = std::strtoul(argv[2], &pEnd, 10);
        size_t num_items = std::strtoul(argv[3], &pEnd, 10);
        size_t num_hashes = std::strtoul(argv[4], &pEnd, 10);
        generate_dummy_bloom(path, signature_size, ((num_items + 7) / 8), num_hashes);
    } else {
        std::cout << "wrong number of args: path signature_size block_size num_hashes" << std::endl;
    }
}
