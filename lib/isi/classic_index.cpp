#include <experimental/filesystem>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <util.hpp>
#include <file/util.hpp>
#include "timer.hpp"
#include "classic_index.hpp"
#include "kmer.hpp"

namespace isi {

    void classic_index::create_hashes(const void* input, size_t len, size_t signature_size, size_t num_hashes,
                                     const std::function<void(size_t)>& callback) {
        for (unsigned int i = 0; i < num_hashes; i++) {
            size_t hash = XXH32(input, len, i);
            callback(hash % signature_size);
        }
    }

    void classic_index::process(const std::vector<std::experimental::filesystem::path>& paths, const std::experimental::filesystem::path& out_file, timer& t) {
        sample<31> s;
        for (size_t i = 0; i < paths.size(); i++) {
            t.active("read");
            file::deserialize(paths[i], s);
            t.active("process");
            #pragma omp parallel for
            for (size_t j = 0; j < s.data().size(); j++) {
                create_hashes(s.data().data() + j, 8, m_signature_size, m_num_hashes, [&](size_t hash) {
                    classic_index::set_bit(hash, i);
                });
            }
        }
        t.active("write");
        std::vector<std::string> file_names(paths.size());
//        for (size_t i = 0; i < paths.size(); i++) {
//            file_names[i] = paths[i].stem().string();
//        }
        std::transform(paths.begin(), paths.end(), file_names.begin(), [](const auto& p) {
            return p.stem().string();
        });
        file::serialize(out_file, *this, file_names);
        t.stop();
    }

    void classic_index::create_from_samples(const std::experimental::filesystem::path &in_dir, const std::experimental::filesystem::path &out_dir,
                                                size_t signature_size, size_t block_size, size_t num_hashes) {
        timer t;
        classic_index bf(signature_size, block_size, num_hashes);
        bulk_process_files(in_dir, out_dir, 8 * block_size, file::sample_header::file_extension, file::classic_index_header::file_extension,
                           [&](const std::vector<std::experimental::filesystem::path>& paths, const std::experimental::filesystem::path& out_file) {
                               bf.process(paths, out_file, t);
                           });
        std::cout << t;
    }


    void classic_index::combine(std::vector<std::pair<std::ifstream, size_t>>& ifstreams, const std::experimental::filesystem::path& out_file,
                               size_t signature_size, size_t block_size, size_t num_hash, timer& t, const std::vector<std::string>& file_names) {
        std::ofstream ofs;
        file::classic_index_header h(signature_size, block_size, num_hash, file_names);
        file::serialize_header(ofs, out_file, h);

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

    bool classic_index::combine(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                                             size_t signature_size, size_t num_hashes, size_t batch_size) {
        timer t;
        std::vector<std::pair<std::ifstream, size_t>> ifstreams;
        std::vector<std::string> file_names;
        bool all_combined = bulk_process_files(in_dir, out_dir, batch_size, file::classic_index_header::file_extension, file::classic_index_header::file_extension,
                           [&](const std::vector<std::experimental::filesystem::path>& paths, const std::experimental::filesystem::path& out_file) {
                               size_t new_block_size = 0;
                               for (size_t i = 0; i < paths.size(); i++) {
                                   ifstreams.emplace_back(std::make_pair(std::ifstream(), 0));
                                   auto h = file::deserialize_header<file::classic_index_header>(ifstreams.back().first, paths[i]);
                                   assert(h.signature_size() == signature_size);
                                   assert(h.num_hashes() == num_hashes);
                                   if (i < paths.size() - 2) {
                                       //todo doesnt work because of padding for compact, which means there could be two files with less file_names
                                       //todo quickfix with -2 to allow for paddding
//                                       assert(h.file_names().size() == 8 * h.block_size());
                                   }
                                   ifstreams.back().second = h.block_size();
                                   new_block_size += h.block_size();
                                   std::copy(h.file_names().begin(), h.file_names().end(), std::back_inserter(file_names));
                               }
                               combine(ifstreams, out_file, signature_size, new_block_size, num_hashes, t, file_names);
                               ifstreams.clear();
                               file_names.clear();
                           });
        std::cout << t;
        return all_combined;
    }

    classic_index::classic_index(uint64_t signature_size, uint64_t block_size, uint64_t num_hashes)
            : m_signature_size(signature_size), m_block_size(block_size), m_num_hashes(num_hashes),
              m_data(signature_size * block_size) {}

    void classic_index::set_bit(size_t pos, size_t bit_in_block) {
        m_data[m_block_size * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
    }

    bool classic_index::is_set(size_t pos, size_t bit_in_block) {
        uint8_t b = m_data[m_block_size * pos + bit_in_block / 8];
        return (b & (1 << (bit_in_block % 8))) != 0;
    };
    bool classic_index::contains(const kmer<31>& kmer, size_t bit_in_block) {
        assert(bit_in_block < 8 * m_block_size);
        for (unsigned int k = 0; k < m_num_hashes; k++) {
            size_t hash = XXH32(&kmer.data(), 8, k);
            if (!is_set(hash % m_signature_size, bit_in_block)) {
                return false;
            }
        }
        return true;
    }

    uint64_t classic_index::signature_size() const {
        return m_signature_size;
    }

    void classic_index::signature_size(uint64_t signature_size) {
        m_signature_size = signature_size;
    }

    uint64_t classic_index::block_size() const {
        return m_block_size;
    }

    void classic_index::block_size(uint64_t block_size) {
        m_block_size = block_size;
    }

    uint64_t classic_index::num_hashes() const {
        return m_num_hashes;
    }

    void classic_index::num_hashes(uint64_t num_hashes) {
        m_num_hashes = num_hashes;
    }

    const std::vector<uint8_t>& classic_index::data() const {
        return m_data;
    }

    std::vector<uint8_t>& classic_index::data() {
        return m_data;
    }

    void classic_index::generate_dummy(const std::experimental::filesystem::path& p, size_t signature_size, size_t block_size, size_t num_hashes) {
        std::vector<std::string> file_names;
        for(size_t i = 0; i < 8 * block_size; i++) {
            file_names.push_back("file_" + std::to_string(i));
        }
        isi::file::classic_index_header h(signature_size, block_size, num_hashes, file_names);
        std::ofstream ofs;
        isi::file::serialize_header(ofs, p, h);

        for (size_t i = 0; i < signature_size * block_size; i += 4) {
            int rnd = std::rand();
            ofs.write(reinterpret_cast<char*>(&rnd), 4);
        }
    }
};
