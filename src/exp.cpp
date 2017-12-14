#include <isi/query/classic_index/base.hpp>
#include <isi/query/classic_index/mmap.hpp>
#include <zconf.h>
#include <isi/query/compact_index/mmap.hpp>

void run(isi::query::classic_index::base& s, size_t query_len, size_t num_iterations, size_t num_warmup_iterations) {
    std::vector<std::pair<uint16_t, std::string>> result;
    sync();
    for (size_t i = 0; i < num_warmup_iterations; i++) {
        std::string query = isi::random_sequence(query_len, std::rand());
        s.search(query, 31, result);
    }

    isi::timer t;
    for (size_t i = 0; i < num_iterations; i++) {
        std::string query = isi::random_sequence(query_len, std::rand());
        t.active("total");
        s.search(query, 31, result);
        t.stop();
    }
    t.stop();
    std::cout << s.get_timer() << std::endl;
    std::cout << t << std::endl;
}

int main(int argc, char **argv) {
    size_t query_len;
    size_t num_iterations;
    size_t num_warmup_iterations;
    std::experimental::filesystem::path p;

    query_len = 1000;
    num_iterations = 100;
    p = "/users/flo/projects/thesis/data/performance_bloom/large.cla_idx.isi";
#ifdef NO_SIMD
    std::cout << "SIMD is disabled" << std::endl;
#endif

#ifdef __linux__
    isi::query::compact_index::aio server(p);
#else
    isi::query::classic_index::mmap server(p);
#endif

    run(server, query_len, num_iterations, num_warmup_iterations);
}
