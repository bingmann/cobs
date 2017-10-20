#include <boost/filesystem.hpp>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <util.hpp>
#include <file/util.hpp>
#include "timer.hpp"
#include "bss.hpp"
#include "kmer.hpp"

namespace genome {

    void bss::create_hashes(const void* input, size_t len, size_t signature_size, size_t num_hashes,
                                     const std::function<void(size_t)>& callback) {
        for (unsigned int i = 0; i < num_hashes; i++) {
            size_t hash = XXH32(input, len, i);
            callback(hash % signature_size);
        }
    }

    void bss::process(const std::vector<boost::filesystem::path>& paths, const boost::filesystem::path& out_file, timer& t) {
        sample<31> s;
        for (size_t i = 0; i < paths.size(); i++) {
            t.active("read");
            file::deserialize(paths[i], s);
            t.active("process");
            for (const auto& kmer: s.data()) {
                create_hashes(&kmer, kmer.size, m_signature_size, m_num_hashes, [&](size_t hash) {
                    bss::set_bit(hash, i);
                });
            }
        }
        t.active("write");
        std::vector<std::string> file_names(8 * m_block_size);
        std::transform(paths.begin(), paths.end(), file_names.begin(), [](const auto& p) {
            return p.stem().string();
        });
        file::serialize(out_file, *this, file_names);
        t.stop();
    }

    void bss::create_from_samples(const boost::filesystem::path &in_dir, const boost::filesystem::path &out_dir,
                                                size_t signature_size, size_t block_size, size_t num_hashes) {
        timer t;
        bss bf(signature_size, block_size, num_hashes);
        bulk_process_files(in_dir, out_dir, 8 * block_size, file::sample_header::file_extension, file::bss_header::file_extension,
                           [&](const std::vector<boost::filesystem::path>& paths, const boost::filesystem::path& out_file) {
                               bf.process(paths, out_file, t);
                           });
        std::cout << t;
    }


    void bss::combine(std::vector<std::pair<std::ifstream, size_t>>& ifstreams, const boost::filesystem::path& out_file,
                               size_t signature_size, size_t block_size, size_t num_hash, timer& t, const std::vector<std::string>& file_names) {
        std::ofstream ofs;
        file::bss_header bssh(signature_size, block_size, num_hash, file_names);
        file::serialize_header(ofs, out_file, bssh);

        std::vector<char> block(block_size);
        for (size_t i = 0; i < signature_size; i++) {
            size_t pos = 0;
            t.active("read");
            for (auto& ifs: ifstreams) {
                ifs.first.read(&(*(block.begin() + pos)), ifs.second);
                pos += ifs.second;
            }
            t.active("write");
            ofs.write(&(*block.begin()), block_size);
        }
        t.stop();
    }

    bool bss::combine_bss(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir,
                                             size_t signature_size, size_t num_hashes, size_t batch_size) {
        timer t;
        std::vector<std::pair<std::ifstream, size_t>> ifstreams;
        std::vector<std::string> file_names;
        bool all_combined = bulk_process_files(in_dir, out_dir, batch_size, file::bss_header::file_extension, file::bss_header::file_extension,
                           [&](const std::vector<boost::filesystem::path>& paths, const boost::filesystem::path& out_file) {
                               size_t new_block_size = 0;
                               for (size_t i = 0; i < paths.size(); i++) {
                                   ifstreams.emplace_back(std::make_pair(std::ifstream(), 0));
                                   auto bssh = file::deserialize_header<file::bss_header>(ifstreams.back().first, paths[i]);
                                   assert(bssh.signature_size() == signature_size);
                                   assert(bssh.num_hashes() == num_hashes);
                                   if (i < paths.size() - 1) {
                                       assert(bssh.file_names().size() == 8 * bssh.block_size());
                                   }
                                   ifstreams.back().second = bssh.block_size();
                                   new_block_size += bssh.block_size();
                                   std::copy(bssh.file_names().begin(), bssh.file_names().end(), std::back_inserter(file_names));
                               }
                               combine(ifstreams, out_file, signature_size, new_block_size, num_hashes, t, file_names);
                               ifstreams.clear();
                               file_names.clear();
                           });
        std::cout << t;
        return all_combined;
    }

    bss::bss(uint64_t signature_size, uint64_t block_size, uint64_t num_hashes)
            : m_signature_size(signature_size), m_block_size(block_size), m_num_hashes(num_hashes),
              m_data(signature_size * block_size) {}

    void bss::set_bit(size_t pos, size_t bit_in_block) {
        m_data[m_block_size * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
    }

    bool bss::is_set(size_t pos, size_t bit_in_block) {
        byte b = m_data[m_block_size * pos + bit_in_block / 8];
        return (b & (1 << (bit_in_block % 8))) != 0;
    };
    bool bss::contains(const kmer<31>& kmer, size_t bit_in_block) {
        assert(bit_in_block < 8 * m_block_size);
        for (unsigned int k = 0; k < m_num_hashes; k++) {
            size_t hash = XXH32(&kmer.data(), 8, k);
            if (!is_set(hash % m_signature_size, bit_in_block)) {
                return false;
            }
        }
        return true;
    }

    uint64_t bss::signature_size() const {
        return m_signature_size;
    }

    void bss::signature_size(uint64_t signature_size) {
        m_signature_size = signature_size;
    }

    uint64_t bss::block_size() const {
        return m_block_size;
    }

    void bss::block_size(uint64_t block_size) {
        m_block_size = block_size;
    }

    uint64_t bss::num_hashes() const {
        return m_num_hashes;
    }

    void bss::num_hashes(uint64_t num_hashes) {
        m_num_hashes = num_hashes;
    }

    const std::vector<byte>& bss::data() const {
        return m_data;
    }

    std::vector<byte>& bss::data() {
        return m_data;
    }
};
