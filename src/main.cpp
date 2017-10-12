#include <boost/filesystem/path.hpp>
#include <server/server.hpp>
#include <server/server_mmap.hpp>
#include <server/server_ifs.hpp>
#include <frequency.hpp>
#include <helpers.hpp>
#include <file/msbf_header.hpp>
#include <omp.h>

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

void run(genome::server& s, size_t query_len, const boost::filesystem::path& p, std::vector<std::pair<uint16_t, std::string>>& result) {
    genome::timer t;
    t.active("total");
    for (size_t i = 0; i < 1; i++) {
        s.search_bloom_filter<31>(genome::random_query(query_len), result);
    }
    t.stop();
    std::cout << s.get_timer() << std::endl;
    std::cout << t << std::endl;
}

void server() {
    std::vector<std::pair<uint16_t, std::string>> result_1;
    std::vector<std::pair<uint16_t, std::string>> result_2;
    boost::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.g_blo");
    genome::server_mmap s_mmap(p);
    genome::server_ifs s_ifs(p);
    size_t query_len = 900;
    run(s_mmap, query_len, p, result_1);
//    run(s_ifs, query_len, p, result_1);
    int a = 0;
}

int main(int argc, char** argv) {
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
    server();

//    genome::file::msbf_header h;
//    boost::filesystem::path p("/users/flo/projects/thesis/data/tests.g_mfs");
//    std::ofstream ofs;
//    genome::file::serialize_header(ofs, p, h);
}
