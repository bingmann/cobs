#include <experimental/filesystem>
#include <isi/query/classic_index/base.hpp>
#include <isi/query/classic_index/mmap.hpp>
#include <isi/query/classic_index/ifs.hpp>
#include <isi/frequency.hpp>
#include <omp.h>

void generate_test_bloom(std::experimental::filesystem::path p) {
    size_t signature_size = 10000000;
    size_t block_size = 8000;
    size_t num_hashes = 3;
    std::vector<std::string> file_names;
    for(size_t i = 0; i < 8 * block_size; i++) {
        file_names.push_back("file_" + std::to_string(i));
    }
    isi::file::classic_index_header h(signature_size, block_size, num_hashes, file_names);
    std::ofstream ofs;
    isi::file::serialize_header(ofs, p, h);

    for (size_t i = 0; i < signature_size * block_size; i++) {
        char rnd = std::rand();
        ofs.write(&rnd, 1);
    }
}

void run(isi::query::classic_index::base& s, size_t query_len, std::vector<std::pair<uint16_t, std::string>>& result) {
    sync();
    isi::timer t;
    t.active("total");
//    for (size_t i = 0; i < 100; i++) {
        s.search(isi::random_sequence(query_len), 31, result, 10);
//    }
    t.stop();
    std::cout << s.get_timer() << std::endl;
    std::cout << t << std::endl;
}

void server() {
    std::vector<std::pair<uint16_t, std::string>> result_1;
    std::vector<std::pair<uint16_t, std::string>> result_2;
    std::vector<std::pair<uint16_t, std::string>> result_3;
    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.cla_idx.isi");

    isi::query::classic_index::mmap s_mmap(p);
    isi::query::classic_index::ifs s_ifs(p);
//    isi::query::classic_index::asio s_asio(p);
    size_t query_len = 1000;
//    run(s_ifs, query_len, p, result_1);
    run(s_mmap, query_len, result_1);
//    run(s_stxxl, query_len, result_3);
}

int main() {
//    generate_test_bloom("/users/flo/projects/thesis/data/performance_bloom/large.cla_idx.isi");
//    std::ifstream ifs("/Users/flo/freq.txt");
//    std::ofstream ofs("/Users/flo/freq_sum_2.txt");
//    std::string line;
//    uint32_t sum = 0;
//    while (std::getline(ifs, line)) {
//        std::vector<std::string> strs;
//        boost::split(strs, line, boost::is_any_of(","));
//        uint32_t a = std::stoi(strs[0]);
//        sum += std::stoi(strs[1]);
//        ofs << a / (double) 19000 << "," << sum << "\n";
//    }
    server();

//    std::cout << getpagesize() << std::endl;

//    std::vector<bool> test(10000000);
//    for (size_t i = 0; i < 2 * test.size() / 5.262; i++) {
//        test[std::rand() % test.size()] = true;
//    }
//
//    int count = 0;
//    for (auto t: test) {
//        if (t) {
//            count++;
//        };
//    }
//
//    uint64_t size = isi::compact_index::calc_signature_size(1000, 2, 0.1);
//    int a = 0;
//    isi::compact_index::create_folders("/users/flo/projects/thesis/data/compact_index_out", "/users/flo/projects/thesis/data/compact_index_out_2", 64);
//    isi::compact_index::create_classic_indexs_from_samples("/users/flo/projects/thesis/data/compact_index_out_2", "/users/flo/projects/thesis/data/compact_index_out_3", 32, 1, 0.3);
//    isi::compact_index::combine_classic_indexs("/users/flo/projects/thesis/data/compact_index_out_3", "/users/flo/projects/thesis/data/compact_index_out_4", 32);
//    isi::compact_index::create_folders("/users/flo/projects/thesis/data/compact_index_in", "/users/flo/projects/thesis/data/compact_index", 16);
//    isi::compact_index::create_compact_index_from_samples("/users/flo/projects/thesis/data/compact_index", 8, 1, 0.3);
//    isi::file::compact_index_header h;
//    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/tests.g_mfs");
//    std::ofstream ofs;
//    isi::file::serialize_header(ofs, p, h);
}
