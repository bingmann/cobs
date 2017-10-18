#include <boost/filesystem/path.hpp>
#include <server/server.hpp>
#include <server/server_mmap.hpp>
#include <server/server_ifs.hpp>
#include <frequency.hpp>
#include <helpers.hpp>
#include <file/msbf_header.hpp>
#include <omp.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/syscall_file.h>
#include <server/server_stxxl.hpp>
#include <msbf.hpp>

void generate_test_bloom(boost::filesystem::path p) {
    size_t bloom_filter_size = 10000000;
    size_t block_size = 8000;
    size_t num_hashes = 3;
    std::vector<std::string> file_names;
    for(size_t i = 0; i < 8 * block_size; i++) {
        file_names.push_back("file_" + std::to_string(i));
    }
    genome::file::bloom_filter_header bfh(bloom_filter_size, block_size, num_hashes, file_names);
    std::ofstream ofs;
    genome::file::serialize_header(ofs, p, bfh);

    for (size_t i = 0; i < bloom_filter_size * block_size; i++) {
        char rnd = std::rand();
        ofs.write(&rnd, 1);
    }
}

void run(genome::server& s, size_t query_len, std::vector<std::pair<uint16_t, std::string>>& result) {
    sync();
    genome::timer t;
    t.active("total");
    for (size_t i = 0; i < 100; i++) {
        s.search_bloom_filter(genome::random_query(query_len), 31, result, 10);
    }
    t.stop();
    std::cout << s.get_timer() << std::endl;
    std::cout << t << std::endl;
}

void server() {
    std::vector<std::pair<uint16_t, std::string>> result_1;
    std::vector<std::pair<uint16_t, std::string>> result_2;
    std::vector<std::pair<uint16_t, std::string>> result_3;
    boost::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.g_blo");
    genome::server_mmap s_mmap(p);
    genome::server_ifs s_ifs(p);
    genome::server_stxxl s_stxxl(p);
    size_t query_len = 900;
//    run(s_ifs, query_len, p, result_1);
    run(s_mmap, query_len, result_1);
//    run(s_stxxl, query_len, result_3);
}

int main() {
//    generate_test_bloom("/users/flo/projects/thesis/data/performance_bloom/large.g_blo");
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
//    server();

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
//    uint64_t size = genome::msbf::calc_bloom_filter_size(1000, 2, 0.1);
//    int a = 0;
//    genome::msbf::create_folders("/users/flo/projects/thesis/data/msbf_out", "/users/flo/projects/thesis/data/msbf_out_2", 64);
//    genome::msbf::create_bloom_filters_from_samples("/users/flo/projects/thesis/data/msbf_out_2", "/users/flo/projects/thesis/data/msbf_out_3", 32, 1, 0.3);
//    genome::msbf::combine_bloom_filters("/users/flo/projects/thesis/data/msbf_out_3", "/users/flo/projects/thesis/data/msbf_out_4", 32);
    genome::msbf::create_folders("/users/flo/projects/thesis/data/msbf_in", "/users/flo/projects/thesis/data/msbf", 16);
    genome::msbf::create_msbf_from_samples("/users/flo/projects/thesis/data/msbf", 8, 1, 0.3);
//    genome::file::msbf_header h;
//    boost::filesystem::path p("/users/flo/projects/thesis/data/tests.g_mfs");
//    std::ofstream ofs;
//    genome::file::serialize_header(ofs, p, h);
}
