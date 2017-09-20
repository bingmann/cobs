#include <frequency.hpp>

int main(int argc, char** argv) {
    std::string in_dir = "test/resources/cortex/input/";
    std::string out_dir = "test/out/cortex/";
    if (argc == 3) {
        in_dir = argv[1];
        out_dir = argv[2];
    }
    frequency::process_all_in_directory<frequency::bin_pq_element>(in_dir, out_dir);
//    frequency::process_all_in_directory<frequency::freq_pq_element>(in_dir, out_dir);
}
