/*******************************************************************************
 * src/main_temp.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/cortex_file.hpp>
#include <cobs/query/classic_index/base.hpp>
#include <cobs/query/classic_index/mmap.hpp>
#include <experimental/filesystem>

#include <omp.h>
#include <unistd.h>

void generate_test_bloom(std::experimental::filesystem::path p) {
    size_t signature_size = 10000000;
    size_t row_size = 8000;
    size_t num_hashes = 3;
    std::vector<std::string> file_names;
    for (size_t i = 0; i < 8 * row_size - 7; i++) {
        file_names.push_back("file_" + std::to_string(i));
    }
    cobs::ClassicIndexHeader h(signature_size, num_hashes, file_names);
    std::ofstream ofs;
    cobs::serialize_header(ofs, p, h);

    for (size_t i = 0; i < signature_size * row_size; i++) {
        char rnd = std::rand();
        ofs.write(&rnd, 1);
    }
}

void run(cobs::query::classic_index::base& s, size_t query_len, std::vector<std::pair<uint16_t, std::string> >& result) {
    sync();
    cobs::Timer t;
    t.active("total");
//    for (size_t i = 0; i < 100; i++) {
    s.search(cobs::random_sequence(query_len, 1234), 31, result, 10);
//    }
    t.stop();
    std::cout << s.get_timer() << std::endl;
    std::cout << t << std::endl;
}

void server() {
    std::vector<std::pair<uint16_t, std::string> > result_1;
    std::vector<std::pair<uint16_t, std::string> > result_2;
    std::vector<std::pair<uint16_t, std::string> > result_3;
    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.cobs_classic");

    cobs::query::classic_index::mmap s_mmap(p);
//    cobs::query::classic_index::asio s_asio(p);
    size_t query_len = 1000;
    run(s_mmap, query_len, result_1);
//    run(s_stxxl, query_len, result_3);
}

template <class T>
int a() {
    return 5;
}

int main() {
//    std::string in = "test/a";
//    std::string out = "test/b";
//    cobs::cortex::process_all_in_directory<31>(in, out);
    generate_test_bloom("/users/flo/projects/thesis/data/performance_bloom/large.cobs_classic");
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
//    uint64_t size = cobs::compact_index::calc_signature_size(1000, 2, 0.1);
//    int a = 0;
//    cobs::compact_index::create_folders("/users/flo/projects/thesis/data/compact_index_out", "/users/flo/projects/thesis/data/compact_index_out_2", 64);
//    cobs::compact_index::create_classic_indexs_from_documents("/users/flo/projects/thesis/data/compact_index_out_2", "/users/flo/projects/thesis/data/compact_index_out_3", 32, 1, 0.3);
//    cobs::compact_index::combine_classic_indexs("/users/flo/projects/thesis/data/compact_index_out_3", "/users/flo/projects/thesis/data/compact_index_out_4", 32);
//    cobs::compact_index::create_folders("/users/flo/projects/thesis/data/compact_index_in", "/users/flo/projects/thesis/data/compact_index", 16);
//    cobs::compact_index::create_compact_index_from_documents("/users/flo/projects/thesis/data/compact_index", 8, 1, 0.3);
//    cobs::file::compact_index_header h;
//    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/tests.g_mfs");
//    std::ofstream ofs;
//    cobs::file::serialize_header(ofs, p, h);
}

/******************************************************************************/
