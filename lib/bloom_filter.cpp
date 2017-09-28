#include <boost/filesystem.hpp>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <helpers.hpp>
#include <file/util.hpp>
#include "timer.hpp"
#include "bloom_filter.hpp"
#include "kmer.hpp"

namespace genome {

    void bloom_filter::process(const std::vector<boost::filesystem::path> &paths, const boost::filesystem::path &out_file, timer &t) {
        sample<31> s;
        for (size_t i = 0; i < paths.size(); i++) {
            t.start();
            file::deserialize(paths[i], s);
            t.next();
            for (const auto& kmer: s.data()) {
                for (unsigned int k = 0; k < m_num_hashes; k++) {
                    size_t hash = XXH32(&kmer, kmer.size, k);
                    bloom_filter::set_bit(hash % m_bloom_filter_size, i);
                }
            }
            t.end();
        }
        t.start();
        t.next();
        t.next();
        file::serialize(out_file, *this);
        t.end();
    }

    void bloom_filter::process_all_in_directory(const boost::filesystem::path &in_dir, const boost::filesystem::path &out_dir,
                                                size_t bloom_filter_size, size_t block_size, size_t num_hashes) {
        timer t = {"read", "process", "write"};
        boost::filesystem::create_directories(out_dir);
        boost::filesystem::recursive_directory_iterator it(in_dir), end;
        std::vector<boost::filesystem::path> sorted_paths;
        std::copy(it, end, std::back_inserter(sorted_paths));
        std::sort(sorted_paths.begin(), sorted_paths.end());

        std::string first_filename;
        std::string last_filename;

        bloom_filter bf(bloom_filter_size, block_size, num_hashes);

        std::vector<boost::filesystem::path> paths;
        for (size_t i = 0; i < sorted_paths.size(); i++) {
            if (boost::filesystem::is_regular_file(sorted_paths[i]) && sorted_paths[i].extension() == file::sample_header::file_extension) {
                std::string filename = sorted_paths[i].stem().string();
                if (first_filename.empty()) {
                    first_filename = filename;
                }
                last_filename = filename;
                paths.push_back(sorted_paths[i]);
            }
            if (paths.size() == 8 * block_size || (!paths.empty() && i + 1 == sorted_paths.size())) {
                boost::filesystem::path out_file = out_dir / ("[" + first_filename + "-" + last_filename + "]" + file::bloom_filter_header::file_extension);
                if (!boost::filesystem::exists(out_file)) {
                    bf.process(paths, out_file, t);
                } else {
                    std::cout << "file exists - " << out_file.string() << std::endl;
                }
                paths.clear();
                first_filename.clear();
            }
        }
        std::cout << t;
    }

    bloom_filter::bloom_filter(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes)
            : m_bloom_filter_size(bloom_filter_size), m_block_size(block_size), m_num_hashes(num_hashes),
              m_data(bloom_filter_size * block_size) {}

    void bloom_filter::set_bit(size_t pos, size_t bit_in_block) {
        m_data[m_block_size * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
    }

    bool bloom_filter::is_set(size_t pos, size_t bit_in_block) {
        byte b = m_data[m_block_size * pos + bit_in_block / 8];
        return  (b & (1 << (bit_in_block % 8))) != 0;
    };
    bool bloom_filter::contains(const kmer<31>& kmer, size_t bit_in_block) {
        assert(bit_in_block < 8 * m_block_size);
        for (unsigned int k = 0; k < m_num_hashes; k++) {
            size_t hash = XXH32(&kmer.data(), 8, k);
            if (!is_set(hash % m_bloom_filter_size, bit_in_block)) {
                return false;
            }
        }
        return true;
    }

    uint64_t bloom_filter::bloom_filter_size() const {
        return m_bloom_filter_size;
    }

    void bloom_filter::bloom_filter_size(uint64_t bloom_filter_size) {
        m_bloom_filter_size = bloom_filter_size;
    }

    uint64_t bloom_filter::block_size() const {
        return m_block_size;
    }

    void bloom_filter::block_size(uint64_t block_size) {
        m_block_size = block_size;
    }

    uint64_t bloom_filter::num_hashes() const {
        return m_num_hashes;
    }

    void bloom_filter::num_hashes(uint64_t num_hashes) {
        m_num_hashes = num_hashes;
    }

    const std::vector<byte>& bloom_filter::data() const {
        return m_data;
    }

    std::vector<byte>& bloom_filter::data() {
        return m_data;
    }
};
