#include <cobs/construction/compact_index.hpp>
#include <cobs/util/parameters.hpp>

namespace cobs::compact_index {
    std::string pad_directory_number(size_t index) {
        std::stringstream ss;
        ss << std::setw(6) << std::setfill('0') << index;
        return ss.str();
    }

    void create_folders(const fs::path& in_dir, const fs::path& out_dir, uint64_t page_size) {
        std::vector<fs::path> paths;
        fs::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return cobs::file::file_is<cobs::file::sample_header>(p);
        });
        std::sort(paths.begin(), paths.end(), [](const auto& p1, const auto& p2) {
            return fs::file_size(p1) < fs::file_size(p2);
        });

        size_t batch = 1;
        size_t i = 0;
        fs::path sub_out_dir;
        for(const auto& p: paths) {
            if (i % (8 * page_size) == 0) {
                sub_out_dir = out_dir / fs::path("/samples/" + pad_directory_number(batch));
                fs::create_directories(sub_out_dir);
                batch++;
            }
            fs::rename(p, sub_out_dir / p.filename());
            i++;
        }
    }

//    void create_from_samples(const fs::path& in_dir, const fs::path& out_dir, size_t batch_size,
//                             size_t num_hashes, double false_positive_probability, uint64_t page_size) {
//        size_t num_files = 0;
//        size_t max_file_size = 0;
//        for (fs::directory_iterator sub_it(in_dir), end; sub_it != end; sub_it++) {
//            if (cobs::file::file_is<cobs::file::sample_header>(sub_it->path())) {
//                max_file_size = std::max(max_file_size, fs::file_size(sub_it->path()));
//                num_files++;
//            }
//        }
//        size_t signature_size = calc_signature_size(max_file_size / 8, num_hashes, false_positive_probability);
//        classic_index::create_from_samples(in_dir, out_dir / in_dir.filename(), signature_size, num_hashes, batch_size);
//
//        if (num_files != 8 * page_size) {
//            assert(num_files < 8 * page_size);
//            assert(in_dir == paths.back());
//        }
//    }

    void create_classic_index_from_samples(const fs::path& in_dir, const fs::path& out_dir, size_t batch_size,
                                                  size_t num_hashes, double false_positive_probability, uint64_t page_size) {
        assert(batch_size % 8 == 0);
        std::vector<fs::path> paths;
        std::copy_if(fs::directory_iterator(in_dir), fs::directory_iterator(), std::back_inserter(paths), [](const auto& p) {
            return fs::is_directory(p);
        });
        std::sort(paths.begin(), paths.end());

        for (const auto& p: paths) {
            size_t num_files = 0;
            size_t max_file_size = 0;
            for (fs::directory_iterator sub_it(p), end; sub_it != end; sub_it++) {
                if (cobs::file::file_is<cobs::file::sample_header>(sub_it->path())) {
                    max_file_size = std::max(max_file_size, fs::file_size(sub_it->path()));
                    num_files++;
                }
            }
            size_t signature_size = calc_signature_size(max_file_size / 8, num_hashes, false_positive_probability);
            classic_index::create_from_samples(p, out_dir / p.filename(), signature_size, num_hashes, batch_size);

            if (num_files != 8 * page_size) {
                assert(num_files < 8 * page_size);
                assert(p == paths.back());
            }
        }
    }

    bool combine_classic_index(const fs::path& in_dir, const fs::path& out_dir, size_t batch_size) {
        bool all_combined = false;
        for (fs::directory_iterator it(in_dir), end; it != end; it++) {
            if (fs::is_directory(it->path())) {
                all_combined = cobs::classic_index::combine(in_dir / it->path().filename(), out_dir / it->path().filename(), batch_size);
            }
        }
        return all_combined;
    }

    void combine(const fs::path& in_dir, const fs::path& out_file, uint64_t page_size) {
        std::vector<fs::path> paths;
        fs::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return cobs::file::file_is<cobs::file::classic_index_header>(p);
        });
        std::sort(paths.begin(), paths.end());

        std::vector<cobs::file::compact_index_header::parameter> parameters;
        std::vector<std::string> file_names;
        for (size_t i = 0; i < paths.size(); i++) {
            auto h = cobs::file::deserialize_header<cobs::file::classic_index_header>(paths[i]);
            parameters.push_back({h.signature_size(), h.num_hashes()});
            file_names.insert(file_names.end(), h.file_names().begin(), h.file_names().end());
            std::cout << i << ": " << h.block_size() << " " << paths[i].string() << std::endl;
            if (i < paths.size() - 1) {
                assert(h.block_size() == page_size);
            } else {
                assert(h.block_size() <= page_size);
            }
        }

        cobs::file::compact_index_header h(parameters, file_names, page_size);
        std::ofstream ofs;
        cobs::file::serialize_header(ofs, out_file, h);

        std::vector<char> buffer(1024 * page_size);
        for (const auto& p: paths) {
            std::ifstream ifs;
            uint64_t block_size = cobs::file::deserialize_header<cobs::file::classic_index_header>(ifs, p).block_size();
            if (block_size == page_size) {
                ofs << ifs.rdbuf();
            } else {
                cobs::stream_metadata smd = get_stream_metadata(ifs);
                uint64_t data_size = smd.end_pos - smd.curr_pos;
                std::vector<char> padding(page_size - block_size, 0);
                while (data_size > 0) {
                    size_t num_uint8_ts = std::min(1024 * block_size, data_size);
                    ifs.read(buffer.data(), num_uint8_ts);
                    data_size -= num_uint8_ts;
                    for (size_t i = 0; i < num_uint8_ts; i += block_size) {
                        ofs.write(buffer.data() + i, block_size);
                        ofs.write(padding.data(), padding.size());
                    }
                }
            }
        }
    }

    void create_from_folders(const fs::path& in_dir, size_t batch_size, size_t num_hashes,
                                                  double false_positive_probability, uint64_t page_size) {
        fs::path samples_dir = in_dir / fs::path("samples/");
        std::string bloom_dir = in_dir.string() +  "/isi_";
        size_t iteration = 1;
        create_classic_index_from_samples(samples_dir, bloom_dir + std::to_string(iteration), batch_size, num_hashes, false_positive_probability, page_size);
        while(!combine_classic_index(bloom_dir + std::to_string(iteration), bloom_dir + std::to_string(iteration + 1), batch_size)) {
            iteration++;
        }
        combine(bloom_dir + std::to_string(iteration + 1), in_dir / ("index" + cobs::file::compact_index_header::file_extension), page_size);
    }

    void create(const fs::path& in_dir, fs::path out_dir,
                       size_t batch_size, size_t num_hashes, double false_positive_probability, uint64_t page_size) {
        create_folders(in_dir, out_dir / fs::path("/samples"), page_size);
        create_from_folders(out_dir, batch_size, num_hashes, false_positive_probability, page_size);
    }
}
