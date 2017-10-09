#include <server/server.hpp>
#include <server/server_ifs.hpp>
#include <server/server_mmap.hpp>


int main(int argc, char** argv) {
    std::vector<std::pair<uint16_t, std::string>> result;
    if (argc == 4) {
        char* pEnd;
        std::string type = argv[1];
        std::string bloom_filter_path = argv[2];
        std::string query = argv[3];

        genome::server_mmap s_mmap(bloom_filter_path);
        s_mmap.search_bloom_filter<31>(query, result);
        std::cout << s_mmap.get_timer();
//        if (type == "mmap") {
//        } else if (type == "ifs") {
//            genome::server_ifs s_ifs(p);
//            genome::frequency::process_all_in_directory<genome::frequency::fre_pq_element>(in_dir, out_dir, batch_size);
//        } else {
//            std::cout << "wrong type: use 'mmap' or 'ifs'" << std::endl;
//        }
    } else {
        std::cout << "wrong number of args: type[mmap/ifs] bloom_filter_path query" << std::endl;
    }
}
