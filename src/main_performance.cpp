#include <cortex.hpp>
#include <numeric>

void sample() {
    genome::cortex::process_all_in_directory<31>("/users/flo/projects/thesis/data/performance", "/users/flo/projects/thesis/data/performance_out");
}

void bss() {
    genome::bss::create_from_samples("/users/flo/projects/thesis/data/performance_out", "/users/flo/projects/thesis/data/performance_blo", 25000000, 4, 3);
}

void bss_2() {
//    genome::bss::combine_bss("/users/flo/projects/thesis/data/performance_blo", "/users/flo/projects/thesis/data/performance_blo_2", 25000000, 3, 7);
}

const uint64_t m_count_expansions[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
void compute_counts(size_t hashes_size, std::vector<uint16_t>& counts, const std::vector<char>& rows, size_t block_size) {
    #pragma omp declare reduction (merge : std::vector<uint16_t> : \
                            std::transform(omp_out.begin(), omp_out.end(), omp_in.begin(), omp_out.begin(), std::plus<uint16_t>())) \
                            initializer(omp_priv = omp_orig)

    #pragma omp parallel for reduction(merge: counts)
    for (uint64_t i = 0; i < hashes_size; i += 1) {
        auto counts_64 = reinterpret_cast<uint64_t*>(counts.data());
        auto rows_8 = rows.data() + i * block_size;
        for (size_t k = 0; k < block_size; k++) {
//            uint128_t a = 0;
            counts_64[2 * k] += m_count_expansions[rows_8[k] & 0xF];
            counts_64[2 * k + 1] += m_count_expansions[rows_8[k] >> 4];
        }
    }
}



int main() {
//    size_t block_size = 50000;
//    size_t hashes_size = 1000;
//    std::vector<uint16_t> counts(8 * block_size, 0);
//    std::vector<char> rows(hashes_size * block_size, 1);
//    std::iota(rows.begin(), rows.end(), 0);
//    genome::timer t;
//    t.active("counts");
//    compute_counts(hashes_size, counts, rows, block_size);
//    t.stop();
//    std::cout << t << std::endl;
//    for (size_t i = 0; i < 24; i++) {
//        std::cout << counts[i] << ", ";
//    }
//    sample();
    bss();
    bss_2();
}
