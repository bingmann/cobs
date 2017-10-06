#include <cortex.hpp>

void sample() {
    genome::cortex::process_all_in_directory<31>("/users/flo/projects/thesis/data/performance", "/users/flo/projects/thesis/data/performance_out");
}

void bloom_filter() {
    genome::bloom_filter::create_from_samples("/users/flo/projects/thesis/data/performance_out", "/users/flo/projects/thesis/data/performance_blo", 15000000, 2, 3);
}

void bloom_filter_2() {
    genome::bloom_filter::combine_bloom_filters("/users/flo/projects/thesis/data/performance_blo", "/users/flo/projects/thesis/data/performance_blo_2", 15000000, 3, 7);
}

int main(int argc, char** argv) {
    clock_t begin = clock();
    sample();
    bloom_filter();
    bloom_filter_2();
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << elapsed_secs << "s" << std::endl;
}
