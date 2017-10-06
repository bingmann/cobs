#include <boost/filesystem/path.hpp>
#include <server/server.hpp>

void generate_test_bloom(boost::filesystem::path p) {
    size_t bloom_filter_size = 10000000;
    size_t block_size = 2000;
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

void run_mmap(const std::string& query, const boost::filesystem::path& p, std::vector<std::pair<std::string, double>>& result) {
    genome::server s(p);
    s.search_bloom_filter<31>(query, result);
}

void run_ifs(const std::string& query, const boost::filesystem::path& p, std::vector<std::pair<std::string, double>>& result) {
    genome::server s(p, 0);
    s.search_bloom_filter_2<31>(query, result);
}

int main(int argc, char** argv) {
    std::vector<std::pair<std::string, double>> result_1;
    std::vector<std::pair<std::string, double>> result_2;
    boost::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.g_blo");
//    generate_test_bloom(p);
    std::string query =
            "AATGATCTACTCTTCCACACGCCAGCATTAGAAACCTAGGTGCAGTTGCATATGGTACTT"
            "TGTGTTCTCATCCATTCCCACTGAATGGATTTTGCTACTGAGCGTAGCGGTACGACTCGA"
            "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
            "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
            "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
            "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
            "CCTATTTCAACAACCTGATCGAAAGGCGTACCAAACCAATGGCGGGAAGGCGCTCATATG"
            "AAAAATCCGAATCTCTCAGGTGGAAGTTCGGTCTCATGGTAAAGGCAATGGCCCTTCCAT";
//    genome::search::search_bloom_filter<31>(, p, result_1);
    genome::timer t = {"process"};
    t.start();
    for (size_t i = 0; i < 50; i++) {
        run_mmap(query, p, result_1);
    }
    t.end();
    std::cout << t << std::endl;
    t.reset();
    t.start();
    for (size_t i = 0; i < 50; i++) {
        run_ifs(query, p, result_2);
    }
    t.end();
    std::cout << t << std::endl;
    int a = 0;
}
