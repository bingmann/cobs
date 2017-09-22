#include <frequency.hpp>

int main(int argc, char** argv) {
    size_t batch_size = 70;
    std::string in_dir = "test/resources/frequency/input/";
    std::string out_dir = "test/out/frequency/";
    if (argc == 4) {
        char* pEnd;
        batch_size = std::strtoul(argv[1], &pEnd, 10);
        in_dir = argv[2];
        out_dir = argv[3];
    }
    frequency::process_all_in_directory<frequency::bin_pq_element>(in_dir, out_dir, batch_size);
//    frequency::process_all_in_directory<frequency::freq_pq_element>(in_dir, out_dir);
}
