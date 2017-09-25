#include <cortex.hpp>

int main(int argc, char** argv) {
    if (argc == 3) {
        std::string in_dir = argv[1];
        std::string out_dir = argv[2];
        genome::cortex::process_all_in_directory<31>(in_dir, out_dir);
    } else {
        std::cout << "wrong number of args: in_dir out_dir" << std::endl;
    }

}
