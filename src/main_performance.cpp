#include <cortex.hpp>

int main(int argc, char** argv) {
    clock_t begin = clock();
    cortex::process_all_in_directory<31>("/users/flo/projects/thesis/data/performance", "/users/flo/projects/thesis/data/performance_out");
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << elapsed_secs << "s" << std::endl;
}
