#include <boost/filesystem.hpp>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <helpers.hpp>
#include "timer.hpp"
#include "bloom_filter.hpp"
#include "kmer.hpp"

namespace genome::bloom_filter {

    template<size_t BLOCK_SIZE>
    void set_bit(std::vector<byte>& v, size_t pos, size_t bit_in_block) {
        v[BLOCK_SIZE * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
    };

    template<size_t BLOOM_FILTER_SIZE, size_t BLOCK_SIZE, size_t NUM_HASHES>
    void process(const std::vector<boost::filesystem::path> &paths, const boost::filesystem::path &out_file, timer &t) {
        std::vector<byte> signatures(BLOOM_FILTER_SIZE * BLOCK_SIZE);

        std::vector<char> v;
        for (size_t i = 0; i < paths.size(); i++) {
            t.start();
            read_file(paths[i], v);
            t.next();
            for (size_t j = 0; j < v.size(); j += 8) {
                for (unsigned int k = 0; k < NUM_HASHES; k++) {
                    size_t hash = XXH32(v.data() + j, 8, k);
                    set_bit<BLOCK_SIZE>(signatures, hash % BLOOM_FILTER_SIZE, i);
                }
            }
            t.end();
        }
        t.start();
        t.next();
        t.next();
        std::ofstream ofs(out_file.string(), std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<char *>(signatures.data()), signatures.size());
        t.end();
    }

    template<size_t BLOOM_FILTER_SIZE, size_t BLOCK_SIZE, size_t NUM_HASHES>
    void process_all_in_directory(const boost::filesystem::path &in_dir, const boost::filesystem::path &out_dir) {
        timer t = {"read", "process", "write"};
        boost::filesystem::create_directories(out_dir);
        boost::filesystem::recursive_directory_iterator it(in_dir), end;
        std::vector<boost::filesystem::path> sorted_paths;
        std::copy(it, end, std::back_inserter(sorted_paths));
        std::sort(sorted_paths.begin(), sorted_paths.end());

        std::string first_filename = "";
        std::string last_filename = "";

        std::vector<boost::filesystem::path> paths;
        for (size_t i = 0; i < sorted_paths.size(); i++) {
            if (boost::filesystem::is_regular_file(sorted_paths[i]) && sorted_paths[i].extension() == ".b") {
                std::string filename = sorted_paths[i].stem().string();
                if (first_filename == "") {
                    first_filename = filename;
                }
                last_filename = filename;
                paths.push_back(sorted_paths[i]);
            }
            if (paths.size() == 8 * BLOOM_FILTER_SIZE || (!paths.empty() && i + 1 == sorted_paths.size())) {
                boost::filesystem::path out_file = out_dir / ("[" + first_filename + "-" + last_filename + "].bl");
                if (!boost::filesystem::exists(out_file)) {
                    process<BLOOM_FILTER_SIZE, BLOCK_SIZE, NUM_HASHES>(paths, out_file, t);
                } else {
                    std::cout << "file exists - " << out_file.string() << std::endl;
                }
                paths.clear();
                first_filename = "";
            }
        }
        std::cout << t;
    }

    template<size_t BLOCK_SIZE>
    bool is_set(const std::vector<byte>& v, size_t pos, size_t bit_in_block) {
        byte b = v[BLOCK_SIZE * pos + bit_in_block / 8];
        return  (b & (1 << (bit_in_block % 8))) != 0;
    };

    template<size_t BLOOM_FILTER_SIZE, size_t BLOCK_SIZE, size_t NUM_HASHES>
    bool contains(const std::vector<byte>& signature, kmer<31> kmer, size_t bit_in_block) {
        for (unsigned int k = 0; k < NUM_HASHES; k++) {
            size_t hash = XXH32(&kmer, 8, k);
            if (!is_set<BLOCK_SIZE>(signature, hash % BLOOM_FILTER_SIZE, bit_in_block)) {
                return false;
            }
        }
        return true;
    }
};
