#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include <isi/query/compact_index/mmap.hpp>

int main(int argc, char **argv) {
    std::string index = "/ssd1/ena_4096.com_idx.isi";
    std::string sample_name = "ERR498185";
    std::vector<std::string> queries;
    for (size_t i = 0; i < 100000; i++) {
        queries.push_back(isi::random_sequence(1030, (size_t) std::rand()));
    }
    isi::query::compact_index::mmap s(index);
    std::map<size_t, size_t> counts;
    std::vector<std::pair<uint16_t, std::string>> result;
    for (size_t i = 0; i < queries.size(); i++) {
        s.search(queries[i], 31, result);
        for (const auto& r: result) {
            if (r.second == sample_name) {
                counts[r.first]++;
                break;
            }
        }
    }

    for (const auto& c: counts) {
        std::cout << c.first << "," << c.second << std::endl;
    }
}

