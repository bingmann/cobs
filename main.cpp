#include <iostream>
#include <fstream>
#include <cmath>

#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/serialization.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <stxxl/vector>

#include "xxhash.h"

std::unique_ptr<boost::dynamic_bitset<>> create_signature(size_t num_bits, size_t num_hashes, const std::string& file_path) {
    std::ifstream ifs(file_path);
    auto signature = std::make_unique<boost::dynamic_bitset<>>(num_bits);
    std::string kmer;
    while (ifs >> kmer) {
        for (unsigned int i = 0; i < num_hashes; i++) {
            size_t hash = XXH32(kmer.c_str(), kmer.length(), i);
            signature->set(hash % num_bits);
        }
    }
    ifs.close();
    return signature;
}


void serialize(const std::unique_ptr<boost::dynamic_bitset<>>& signature, const std::string& file_path) {
    std::ofstream ofs(file_path);
    boost::archive::binary_oarchive ar(ofs);
    ar << signature;
    ofs.close();
}

std::unique_ptr<boost::dynamic_bitset<>> deserialize(const std::string& file_path) {
    std::ifstream ifs(file_path);
    boost::archive::binary_iarchive ia(ifs);
    std::unique_ptr<boost::dynamic_bitset<>> signature;
    ia >> signature;
    return signature;
}

std::unique_ptr<std::vector<stxxl::vector<char>>> create_matrix(size_t num_rows) {
    auto matrix = std::make_unique<std::vector<stxxl::vector<char>>>(num_rows);
    for (size_t i = 0; i < num_rows; i++) {
        stxxl::vector<char> row;
        row.push_back('a');
        row.export_files("/Users/Flo/projects/stxxl/" + std::to_string(i) + "_");
        (*matrix)[i] = row;
    }
    return matrix;
}


int main() {
    auto signature = create_signature(67108864, 7, "/Users/flo/projects/thesis/data/ERR109478.kmer");
    std::cout << signature->count() << std::endl;

//    serialize(signature, "/Users/flo/projects/thesis/data/ERR109478.b");
//    signature = deserialize("/Users/flo/projects/thesis/data/ERR109478.b");

    std::cout << signature->count() << std::endl;

    auto matrix = create_matrix(1000);

    std::cout << "" << std::endl;
}
