#include <frequency.hpp>

int main(int argc, char** argv) {
    if (argc == 5) {
        char* pEnd;
        std::string type = argv[1];
        std::string in_dir = argv[2];
        std::string out_dir = argv[3];
        size_t batch_size = std::strtoul(argv[4], &pEnd, 10);
        if (type == "bin") {
            genome::frequency::process_all_in_directory<genome::frequency::bin_pq_element>(in_dir, out_dir, batch_size);
        } else if (type == "fre") {
            genome::frequency::process_all_in_directory<genome::frequency::fre_pq_element>(in_dir, out_dir, batch_size);
        } else {
            std::cout << "wrong type: use 'bin' or 'fre'" << std::endl;
        }
    } else {
        std::cout << "wrong number of args: type[bin/fre] in_dir out_dir batch_size" << std::endl;
    }
}
