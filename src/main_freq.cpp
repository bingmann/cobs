//
// Created by Florian Gauger on 10.09.2017.
//

#include <queue>
#include <iostream>
#include <vector>
#include <utility>
#include "../lib/helpers.hpp"

using kmer_tuple = std::tuple<uint64_t, unsigned int, std::ifstream*>;

bool get_tuple(kmer_tuple& tuple) {
    uint64_t kmer;
    unsigned int count;
    std::string line;

    if (std::getline(*std::get<2>(tuple), line)) {
        std::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of(","));
        std::get<0>(tuple) = std::stoull(strs[0]);
        std::get<1>(tuple) = std::stol(strs[1]);
        return true;
    } else {
        return false;
    }
};

void print_result(const std::string& path) {
    auto counts = deserialize<std::unordered_map<unsigned int, size_t>>(path);
    std::map<unsigned int, size_t > counts_sorted(counts.begin(), counts.end());
    size_t total_count = 0;
    size_t unique_count = 0;
    for(auto it = counts_sorted.begin(); it != counts_sorted.end(); ++it) {
        std::cout << it->first << "," << it->second << "," << it->first * it->second << "," << it->first * it->second + total_count << std::endl;
        total_count += it->first * it->second;
        unique_count += it->second;
    }
    std::cout << total_count << std::endl;

    double entropy = 0;
    for (auto it = counts_sorted.begin(); it != counts_sorted.end(); ++it) {
        if (entropy == 0) {
            it++;
        }
        double probability = it->first / (double) total_count;
        entropy -= it->second * (probability * log2(probability));
        std::cout << entropy << std::endl;
    }
}

void print_file_sizes(const std::string& path) {
    std::vector<size_t> sizes = get_file_sizes_in_dir(path);
    std::sort(sizes.begin(), sizes.end());
    for (size_t size: sizes) {
        std::cout << size << std::endl;
    }
}


int main(int argc, char* argv[]) {
//    print_file_sizes("/users/flo/projects/thesis/data/bin");
    print_result("/users/flo/projects/thesis/data/freq_total.b");

    std::vector<std::string> fileNames = get_files_in_dir("/users/flo/projects/thesis/data/freq");
    std::vector<std::ifstream> ifstreams(fileNames.size());
    std::transform(fileNames.begin(), fileNames.end(), ifstreams.begin(), [](const auto& fileName){
        return std::ifstream(fileName);
    });
    auto comp = [](const auto& t1, const auto& t2){
        return std::get<0>(t1) > std::get<1>(t1);
    };
    std::priority_queue<kmer_tuple, std::vector<kmer_tuple>, decltype(comp)> pq(comp);

    for (auto& ifs: ifstreams) {
        kmer_tuple tuple;
        std::get<0>(tuple) = 0;
        std::get<1>(tuple) = 0;
        std::get<2>(tuple) = &ifs;

        if(get_tuple(tuple)) {
            pq.push(tuple);
        }
    }

    std::unordered_map<unsigned int, size_t> counts;

    int i = 0;
    uint64_t kmer = UINT64_MAX;
    unsigned int count = 0;
    while(!pq.empty()) {
        auto tuple = pq.top();
        pq.pop();
        if(std::get<0>(tuple) == kmer) {
            count += std::get<1>(tuple);
        } else {
            counts[count]++;
            count = std::get<1>(tuple);
            kmer = std::get<0>(tuple);
        }
        if (get_tuple(tuple)) {
            pq.push(tuple);
        }
        if (i == 100000000) {
            std::cout << kmer << std::endl;
            i = 0;
        }
        i++;
    }
    serialize(counts, "/users/flo/projects/thesis/data/freq_total.b");
}
