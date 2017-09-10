//
// Created by Florian Gauger on 10.09.2017.
//

#include <queue>
#include "../lib/helpers.hpp"

bool get_tuple(std::tuple<uint64_t, unsigned int, std::ifstream>& tuple) {
    uint64_t kmer;
    unsigned int count;
    if (std::get<2>(tuple) >> kmer >> count) {
        std::get<0>(tuple) = kmer;
        std::get<1>(tuple) = count;
        return true;
    } else {
        return false;
    }
};

int main(int argc, char* argv[]) {

    std::vector<std::string> fileNames = get_files_in_dir("/users/flo/projects/thesis/data/freq");
    std::vector<std::ifstream> ifstreams(fileNames.size());
    std::transform(fileNames.begin(), fileNames.end(), ifstreams.begin(), [](const auto& fileName){
        return std::ifstream(fileName);
    });
    std::priority_queue<std::tuple<uint64_t, unsigned int, std::ifstream>> pq;
    for (auto& ifs: ifstreams) {
        auto tuple = std::make_tuple(0, 0, ifs);
        if(get_tuple(tuple)) {
            pq.push(tuple);
        }
    }

    uint64_t kmer = UINT64_MAX;
    unsigned int count = 0;
    while(!pq.empty()) {
        auto tuple = pq.top();
        if (count == 0) {
            count = std::get<1>(tuple);
        } else {
            if(std::get<0>(tuple) == kmer) {
                count += std::get<1>(tuple);
            }
        }
        if (get_tuple(tuple)) {
            pq.push(tuple);
        }
        pq.pop();
    }
}
