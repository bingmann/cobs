#include <cortex.hpp>

void sample() {
    genome::cortex::process_all_in_directory<31>("/users/flo/projects/thesis/data/performance", "/users/flo/projects/thesis/data/performance_out");
}

void bss() {
    genome::bss::create_from_samples("/users/flo/projects/thesis/data/performance_out", "/users/flo/projects/thesis/data/performance_blo", 25000000, 4, 3);
}

void bss_2() {
//    genome::bss::combine_bsss("/users/flo/projects/thesis/data/performance_blo", "/users/flo/projects/thesis/data/performance_blo_2", 15000000, 3, 7);
}

int main() {
    clock_t begin = clock();
    sample();
    bss();
    bss_2();
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << elapsed_secs << "s" << std::endl;
}
