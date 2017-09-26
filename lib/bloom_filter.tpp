#include <boost/filesystem.hpp>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <helpers.hpp>
#include "timer.hpp"
#include "bloom_filter.hpp"
#include "kmer.hpp"

namespace genome::bloom_filter {

    void set_bit(std::vector<byte>& v, size_t pos, size_t bit_in_block, size_t block_size) {
        v[block_size * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
    };

    void process(const std::vector<boost::filesystem::path> &paths, const boost::filesystem::path &out_file, timer &t,
                 size_t bloom_filter_size, size_t block_size, size_t num_hashes) {
        std::vector<byte> signatures(bloom_filter_size * block_size);

        std::vector<char> v;
        for (size_t i = 0; i < paths.size(); i++) {
            t.start();
            read_file(paths[i], v);
            t.next();
            for (size_t j = 0; j < v.size(); j += 8) {
                for (unsigned int k = 0; k < num_hashes; k++) {
                    size_t hash = XXH32(v.data() + j, 8, k);
                    set_bit(signatures, hash % bloom_filter_size, i, block_size);
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

    void process_all_in_directory(const boost::filesystem::path &in_dir, const boost::filesystem::path &out_dir,
                                  size_t bloom_filter_size, size_t block_size, size_t num_hashes) {
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
            if (paths.size() == 8 * bloom_filter_size || (!paths.empty() && i + 1 == sorted_paths.size())) {
                boost::filesystem::path out_file = out_dir / ("[" + first_filename + "-" + last_filename + "].bl");
                if (!boost::filesystem::exists(out_file)) {
                    process(paths, out_file, t, bloom_filter_size, block_size, num_hashes);
                } else {
                    std::cout << "file exists - " << out_file.string() << std::endl;
                }
                paths.clear();
                first_filename = "";
            }
        }
        std::cout << t;
    }

    bool is_set(const std::vector<byte>& v, size_t pos, size_t bit_in_block, size_t block_size) {
        byte b = v[block_size * pos + bit_in_block / 8];
        return  (b & (1 << (bit_in_block % 8))) != 0;
    };

    bool contains(const std::vector<byte>& signature, const kmer<31>& kmer,
                  size_t bit_in_block, size_t bloom_filter_size, size_t block_size, size_t num_hashes) {
        for (unsigned int k = 0; k < num_hashes; k++) {
            size_t hash = XXH32(&kmer, 8, k);
            if (!is_set(signature, hash % bloom_filter_size, bit_in_block, block_size)) {
                return false;
            }
        }
        return true;
    }
};
