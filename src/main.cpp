#include <boost/filesystem/path.hpp>
#include <server/server.hpp>
#include <server/server_mmap.hpp>
#include <server/server_ifs.hpp>
#include <frequency.hpp>
#include <helpers.hpp>

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

void run(genome::server& s, const std::string& query, const boost::filesystem::path& p, std::vector<std::pair<uint16_t, std::string>>& result) {
    for (size_t i = 0; i < 50; i++) {
        s.search_bloom_filter<31>(query, result);
    }
    std::cout << s.get_timer() << std::endl;
}

void server() {
    std::vector<std::pair<uint16_t, std::string>> result_1;
    std::vector<std::pair<uint16_t, std::string>> result_2;
    boost::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.g_blo");
    genome::server_mmap s_mmap(p);
    genome::server_ifs s_ifs(p);
    std::string query =
            "AATGATCTACTCTTCCACACGCCAGCATTAGAAACCTAGGTGCAGTTGCATATGGTACTT"
                    "TGTGTTCTCATCCATTCCCACTGAATGGATTTTGCTACTGAGCGTAGCGGTACGACTCGA"
                    "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
                    "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
                    "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
                    "GTATGGCCTGCCAATTGAGACGATATACTCCCCGATGGCGGATCCACCCGCTGCGGCATG"
                    "CCTATTTCAACAACCTGATCGAAAGGCGTACCAAACCAATGGCGGGAAGGCGCTCATATG"
                    "AAAAATCCGAATCTCTCAGGTGGAAGTTCGGTCTCATGGTAAAGGCAATGGCCCTTCCAT";

    run(s_mmap, query, p, result_1);
    run(s_ifs, query, p, result_1);
    int a = 0;
}

int main(int argc, char** argv) {
}
