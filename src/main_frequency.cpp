#include <frequency.hpp>

int main(int argc, char** argv) {
    std::string in_dir = "test/resources/frequency/input_1/";
    std::string out_dir = "test/out/frequency/";
    size_t batch_size = 70;
    if (argc == 4) {
        char* pEnd;
        in_dir = argv[2];
        out_dir = argv[3];
        batch_size = std::strtoul(argv[1], &pEnd, 10);
    } else {
        std::cout << "wrong number of args: in_dir out_dir batch_size" << std::endl;
    }
    genome::frequency::process_all_in_directory<genome::frequency::bin_pq_element>(in_dir, out_dir, batch_size);
//    frequency::process_all_in_directory<frequency::freq_pq_element>(in_dir, out_dir);
}
