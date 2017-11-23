#include <experimental/filesystem>
#include <xxhash.h>
#include <iostream>
#include <fstream>

#include <isi/util.hpp>
#include <isi/util/file.hpp>
#include <isi/timer.hpp>
#include <isi/construction/classic_index_construction.hpp>
#include <isi/kmer.hpp>

namespace isi::classic_index {
    void create_hashes(const void* input, size_t len, size_t signature_size, size_t num_hashes,
                                     const std::function<void(size_t)>& callback) {
        for (unsigned int i = 0; i < num_hashes; i++) {
            size_t hash = XXH32(input, len, i);
            callback(hash % signature_size);
        }
    }

    namespace {
        void set_bit(std::vector<uint8_t>& data, const isi::file::classic_index_header& h, size_t pos, size_t bit_in_block) {
            data[h.block_size() * pos + bit_in_block / 8] |= 1 << (bit_in_block % 8);
        }

        void process(const std::vector<std::experimental::filesystem::path>& paths,
                     const std::experimental::filesystem::path& out_file, std::vector<uint8_t>& data,
                     isi::file::classic_index_header& h, timer& t) {
            sample<31> s;
            for (size_t i = 0; i < paths.size(); i++) {
                t.active("read");
                file::deserialize(paths[i], s);
                t.active("process");

                #pragma omp parallel for
                for (size_t j = 0; j < s.data().size(); j++) {
                    create_hashes(s.data().data() + j, 8, h.signature_size(), h.num_hashes(), [&](size_t hash) {
                        set_bit(data, h, hash, i);
                    });
                }
            }
            t.active("write");
            h.file_names().resize(paths.size());
            std::transform(paths.begin(), paths.end(), h.file_names().begin(), isi::file::file_name);
            file::serialize(out_file, data, h);
            t.stop();
        }

        void combine(std::vector<std::pair<std::ifstream, size_t>>& ifstreams, const std::experimental::filesystem::path& out_file,
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
    }

    void create_from_samples(const std::experimental::filesystem::path &in_dir, const std::experimental::filesystem::path &out_dir,
                                                size_t signature_size, size_t block_size, size_t num_hashes) {
        timer t;
        isi::file::classic_index_header h(signature_size, block_size, num_hashes);
        std::vector<uint8_t> data(signature_size * block_size);
        bulk_process_files<file::sample_header>(in_dir, out_dir, 8 * block_size, file::classic_index_header::file_extension,
                           [&](const std::vector<std::experimental::filesystem::path>& paths, const std::experimental::filesystem::path& out_file) {
                               process(paths, out_file, data, h, t);
                               std::fill(data.begin(), data.end(), 0);
                               h.file_names().clear();
                           });
        std::cout << t;
    }

    bool combine(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir,
                                             size_t signature_size, size_t num_hashes, size_t batch_size) {
        timer t;
        std::vector<std::pair<std::ifstream, size_t>> ifstreams;
        std::vector<std::string> file_names;
        bool all_combined =
                bulk_process_files<file::classic_index_header>(in_dir, out_dir, batch_size, file::classic_index_header::file_extension,
                                   [&](const std::vector<std::experimental::filesystem::path>& paths, const std::experimental::filesystem::path& out_file) {
                                       size_t new_block_size = 0;
                                       for (size_t i = 0; i < paths.size(); i++) {
                                           ifstreams.emplace_back(std::make_pair(std::ifstream(), 0));
                                           auto h = file::deserialize_header<file::classic_index_header>(ifstreams.back().first, paths[i]);
                                           assert(h.signature_size() == signature_size);
                                           assert(h.num_hashes() == num_hashes);
                                           #ifndef isi_test
                                           if (i < paths.size() - 2) {
                                               //todo doesnt work because of padding for compact, which means there could be two files with less file_names
                                               //todo quickfix with -2 to allow for paddding
                                               assert(h.file_names().size() == 8 * h.block_size());
                                           }
                                           #endif
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

    void create(const std::experimental::filesystem::path& in_dir,
                               const std::experimental::filesystem::path& out_dir,
                               size_t signature_size, size_t block_size, size_t num_hashes, size_t batch_size) {
        create_from_samples(in_dir, out_dir / std::experimental::filesystem::path("1"), signature_size, block_size, num_hashes);
        size_t i = 1;
        while(!combine(out_dir / std::experimental::filesystem::path(std::to_string(i)),
                       out_dir / std::experimental::filesystem::path(std::to_string(i + 1)),
        signature_size, num_hashes, batch_size)) {
            i++;
        }

        std::experimental::filesystem::path index;
        for (std::experimental::filesystem::directory_iterator sub_it(out_dir.string() + "/" + std::to_string(i + 1)), end; sub_it != end; sub_it++) {
            if (isi::file::file_is<isi::file::classic_index_header>(sub_it->path())) {
                index = sub_it->path();
            }
        }
        std::experimental::filesystem::rename(index, out_dir.string() + "/index" + isi::file::classic_index_header::file_extension);
    }

    void create_dummy(const std::experimental::filesystem::path& p, size_t signature_size, size_t block_size, size_t num_hashes) {
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
