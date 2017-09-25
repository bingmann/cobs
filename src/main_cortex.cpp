#include <cortex.hpp>

int main(int argc, char** argv) {
    std::string in_dir = "test/resources/cortex/input_1/";
    std::string out_dir = "test/out/cortex/";
    if (argc == 3) {
        in_dir = argv[1];
        out_dir = argv[2];
    } else {
        std::cout << "wrong number of args: in_dir out_dir" << std::endl;
    }

    genome::cortex::process_all_in_directory<31>(in_dir, out_dir);
}
