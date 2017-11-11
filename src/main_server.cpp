#include <server/bss/base.hpp>
#include <server/bss/ifs.hpp>
#include <server/bss/mmap.hpp>


int main(int argc, char** argv) {
    std::vector<std::pair<uint16_t, std::string>> result;
    if (argc == 4) {
        std::string type = argv[1];
        std::string bss_path = argv[2];
        std::string query = argv[3];

        isi::server::bss::mmap s_mmap(bss_path);
        s_mmap.search(query, 31, result);
        std::cout << s_mmap.get_timer();
//        if (type == "mmap") {
//        } else if (type == "ifs") {
//            isi::server::bss::ifs s_ifs(p);
//            isi::frequency::process_all_in_directory<isi::frequency::fre_pq_element>(in_dir, out_dir, batch_size);
//        } else {
//            std::cout << "wrong type: use 'mmap' or 'ifs'" << std::endl;
//        }
    } else {
        std::cout << "wrong number of args: type[mmap/ifs] bss_path query" << std::endl;
    }
}
