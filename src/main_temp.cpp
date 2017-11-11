#include <experimental/filesystem>
#include <server/bss/base.hpp>
#include <server/bss/mmap.hpp>
#include <server/bss/ifs.hpp>
#include <frequency.hpp>
#include <util.hpp>
#include <file/abss_header.hpp>
#include <omp.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/syscall_file.h>
#include <bit_sliced_signatures/abss.hpp>
#include <server/bss/asio.hpp>

void generate_test_bloom(std::experimental::filesystem::path p) {
    size_t signature_size = 10000000;
    size_t block_size = 8000;
    size_t num_hashes = 3;
    std::vector<std::string> file_names;
    for(size_t i = 0; i < 8 * block_size; i++) {
        file_names.push_back("file_" + std::to_string(i));
    }
    isi::file::bss_header bssh(signature_size, block_size, num_hashes, file_names);
    std::ofstream ofs;
    isi::file::serialize_header(ofs, p, bssh);

    for (size_t i = 0; i < signature_size * block_size; i++) {
        char rnd = std::rand();
        ofs.write(&rnd, 1);
    }
}

void run(isi::server::bss::base& s, size_t query_len, std::vector<std::pair<uint16_t, std::string>>& result) {
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
    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.g_bss");

    isi::server::bss::mmap s_mmap(p);
    isi::server::bss::ifs s_ifs(p);
//    isi::server::bss::asio s_asio(p);
    size_t query_len = 1000;
//    run(s_ifs, query_len, p, result_1);
    run(s_mmap, query_len, result_1);
//    run(s_stxxl, query_len, result_3);
}

int main() {
//    generate_test_bloom("/users/flo/projects/thesis/data/performance_bloom/large.g_bss");
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
//    uint64_t size = isi::abss::calc_signature_size(1000, 2, 0.1);
//    int a = 0;
//    isi::abss::create_folders("/users/flo/projects/thesis/data/abss_out", "/users/flo/projects/thesis/data/abss_out_2", 64);
//    isi::abss::create_bsss_from_samples("/users/flo/projects/thesis/data/abss_out_2", "/users/flo/projects/thesis/data/abss_out_3", 32, 1, 0.3);
//    isi::abss::combine_bsss("/users/flo/projects/thesis/data/abss_out_3", "/users/flo/projects/thesis/data/abss_out_4", 32);
//    isi::abss::create_folders("/users/flo/projects/thesis/data/abss_in", "/users/flo/projects/thesis/data/abss", 16);
//    isi::abss::create_abss_from_samples("/users/flo/projects/thesis/data/abss", 8, 1, 0.3);
//    isi::file::abss_header h;
//    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/tests.g_mfs");
//    std::ofstream ofs;
//    isi::file::serialize_header(ofs, p, h);
}
