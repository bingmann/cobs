#include <frequency.hpp>

int main(int argc, char** argv) {
    std::string in_dir = "test/resources/frequency/input";
    std::string out_dir = "test/out/";
    if (argc == 3) {
        in_dir = argv[2];
        out_dir = argv[3];
    }
    frequency::process_all_in_directory(in_dir, out_dir);
}
