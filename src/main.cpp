#include <iostream>
#include <fstream>
#include <cmath>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/serialization.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>

#include <stxxl/vector>
#include <stxxl/bits/io/syscall_file.h>

#include "xxhash.h"
#include "../lib/kmer.hpp"
#include "../lib/sample.hpp"
#include "../lib/helpers.hpp"

std::unique_ptr<boost::dynamic_bitset<>> create_signature(size_t num_bits, size_t num_hashes, const std::string& file_path) {
//    std::ifstream ifs(file_path);
//    auto signature = std::make_unique<boost::dynamic_bitset<>>(num_bits);
//    std::string kmer;
//    while (ifs >> kmer) {
//        for (unsigned int i = 0; i < num_hashes; i++) {
//            size_t hash = XXH32(kmer.c_str(), kmer.length(), i);
//            signature->set(hash % num_bits);
//        }
//    }
//    ifs.close();
//    return signature;
}


void create_matrix(size_t num_rows, const std::string& path) {
    for (size_t i = 0; i < num_rows; i++) {
        stxxl::syscall_file file(path + std::to_string(i), stxxl::file::RDWR | stxxl::file::CREAT);
        stxxl::vector<char> row(&file);
        for (int j = 0; j < 1000; j++)
        row.push_back('a');
    }
}

template<unsigned int N>
void format_sample(const std::string& cmd, sample<N>& sample) {
    boost::process::ipstream is;
    boost::process::child child(cmd, boost::process::std_out > is);
    sample.init(is, [&](std::string& line) {
        return child.running() && std::getline(is, line) && !line.empty();
    });
    child.wait();
}

void bulk_process(std::string file_paths_string) {
    std::vector<std::string> file_paths;
    boost::split(file_paths, file_paths_string, boost::is_any_of(" "));

    clock_t begin = clock();
    sample<31> sample;
    for (const auto& file_path: file_paths) {
        std::string file_name = file_path.substr(file_path.find("/") + 1);
        file_name = file_name.substr(0, file_name.find("/"));
        std::string cmd = "mccortex31 view -q -k /Users/Flo/projects/thesis/data/" + file_path;
        format_sample(cmd, sample);
        serialize(sample, "/Users/Flo/projects/thesis/data/bin/" + file_name + ".b");
    }
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << elapsed_secs << std::endl;
}

int main(int argc, char** argv) {
    kmer<31> kmer;
    bulk_process(argv[1]);
//    auto signature = create_signature(67108864, 7, "/Users/flo/projects/thesis/data/raw/DRR002056/cleanedDRR002056.kmer");
//    std::cout << signature->count() << std::endl;
//    int signature = 4;
//    serialize(signature, "/Users/flo/projects/thesis/data/ERR109478.b");
//    signature = deserialize("/Users/flo/projects/thesis/data/ERR109478.b");

//    std::cout << signature->count() << std::endl;

//    auto matrix = create_matrix(1000);
//    create_matrix(100, "/Users/Flo/projects/stxxl/");

//    std::cout << file_names_string << std::endl;
}
