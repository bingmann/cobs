#include <frequency.hpp>

int main(int argc, char** argv) {
    if (argc == 4) {
        char* pEnd;
        std::string in_dir = argv[2];
        std::string out_dir = argv[3];
        size_t batch_size = std::strtoul(argv[1], &pEnd, 10);
        genome::frequency::process_all_in_directory<genome::frequency::bin_pq_element>(in_dir, out_dir, batch_size);
    } else {
        std::cout << "wrong number of args: in_dir out_dir batch_size" << std::endl;
    }
}
