#include <cortex.hpp>

int main(int argc, char** argv) {
    std::string in_dir = "test/resources/cortex/input/";
    std::string out_dir = "test/out/cortex/";
    if (argc == 3) {
        in_dir = argv[1];
        out_dir = argv[2];
    }
    cortex::process_all_in_directory<31>(in_dir, out_dir);
}
