#include <isi/construction/compact_index.hpp>
#include <isi/util/parameters.hpp>

namespace isi::compact_index {
    void create_folders(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, uint64_t page_size) {
        std::vector<std::experimental::filesystem::path> paths;
        std::experimental::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return std::experimental::filesystem::is_regular_file(p) && isi::file::file_is<isi::file::sample_header>(p);
        });
        std::sort(paths.begin(), paths.end(), [](const auto& p1, const auto& p2) {
            return std::experimental::filesystem::file_size(p1) < std::experimental::filesystem::file_size(p2);
        });

        size_t batch = 1;
        size_t i = 0;
        std::experimental::filesystem::path sub_out_dir;
        for(const auto& p: paths) {
            if (i % (8 * page_size) == 0) {
                sub_out_dir = out_dir / std::experimental::filesystem::path("/samples/" + std::to_string(batch));
                std::experimental::filesystem::create_directories(sub_out_dir);
                batch++;
            }
            std::experimental::filesystem::rename(p, sub_out_dir / p.filename());
            i++;
        }
    }

    void create_classic_index_from_samples(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, size_t batch_size,
                                                  size_t num_hashes, double false_positive_probability, uint64_t page_size) {
        assert(batch_size % 8 == 0);
        std::vector<std::experimental::filesystem::path> paths;
        std::copy_if(std::experimental::filesystem::directory_iterator(in_dir), std::experimental::filesystem::directory_iterator(), std::back_inserter(paths), [](const auto& p) {
            return std::experimental::filesystem::is_directory(p);
        });
        std::sort(paths.begin(), paths.end());

        for (const auto& p: paths) {
            size_t num_files = 0;
            size_t max_file_size = 0;
            for (std::experimental::filesystem::directory_iterator sub_it(p), end; sub_it != end; sub_it++) {
                if (isi::file::file_is<isi::file::sample_header>(sub_it->path())) {
                    max_file_size = std::max(max_file_size, std::experimental::filesystem::file_size(sub_it->path()));
                    num_files++;
                }
            }
            size_t signature_size = calc_signature_size(max_file_size / 8, num_hashes, false_positive_probability);
            classic_index::create_from_samples(p, out_dir / p.filename(), signature_size, batch_size / 8, num_hashes);

            if (num_files != 8 * page_size) {
                assert(num_files < 8 * page_size);
                assert(p == paths.back());
                isi::file::classic_index_header h(signature_size, (8 * page_size - num_files) / 8, num_hashes);
                std::vector<uint8_t> data(h.signature_size() * h.block_size());
                isi::file::serialize(out_dir / p.filename() / ("padding" + file::classic_index_header::file_extension), data, h);
            }
        }
    }

    bool combine_classic_index(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, size_t batch_size) {
        bool all_combined = false;
        for (std::experimental::filesystem::directory_iterator it(in_dir), end; it != end; it++) {
            if (std::experimental::filesystem::is_directory(it->path())) {
                size_t signature_size = 0;
                size_t num_hashes = 0;
                for (std::experimental::filesystem::directory_iterator bloom_it(it->path()); bloom_it != end; bloom_it++) {
                    if (isi::file::file_is<isi::file::classic_index_header>(bloom_it->path())) {
                        auto h = file::deserialize_header<file::classic_index_header>(bloom_it->path());
                        signature_size = h.signature_size();
                        num_hashes = h.num_hashes();
                        break;
                    }
                }
                if (signature_size != 0) {
                    all_combined = isi::classic_index::combine(in_dir / it->path().filename(), out_dir / it->path().filename(),
                                                               signature_size, num_hashes,
                                                               batch_size);
                } else {
                    std::cerr << "empty directory" << std::endl;
                }
            }
        }
        return all_combined;
    }

    void create_compact_index(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_file, uint64_t page_size) {
        std::vector<std::experimental::filesystem::path> paths;
        std::experimental::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return std::experimental::filesystem::is_regular_file(p) && isi::file::file_is<isi::file::classic_index_header>(p);
        });
        std::sort(paths.begin(), paths.end());

        std::vector<isi::file::compact_index_header::parameter> parameters;
        std::vector<std::string> file_names;
        for (const auto& p: paths) {
            auto h = isi::file::deserialize_header<isi::file::classic_index_header>(p);
            parameters.push_back({h.signature_size(), h.num_hashes()});
            file_names.insert(file_names.end(), h.file_names().begin(), h.file_names().end());
            assert(h.block_size() == page_size);
        }

        isi::file::compact_index_header h(parameters, file_names, page_size);
        std::ofstream ofs;
        isi::file::serialize_header(ofs, out_file, h);

        std::vector<char> buffer(32 * page_size);
        for (const auto& p: paths) {
            std::ifstream ifs;
            isi::file::deserialize_header<isi::file::classic_index_header>(ifs, p);
            isi::stream_metadata smd = get_stream_metadata(ifs);
            size_t data_size = smd.end_pos - smd.curr_pos;
            while(data_size > 0) {
                size_t num_uint8_ts = std::min(buffer.size(), data_size);
                ifs.read(buffer.data(), num_uint8_ts);
                data_size -= num_uint8_ts;
                ofs.write(buffer.data(), num_uint8_ts);
            }
        }
    }

    void create_from_folders(const std::experimental::filesystem::path& in_dir, size_t batch_size, size_t num_hashes,
                                                  double false_positive_probability, uint64_t page_size) {
        std::experimental::filesystem::path samples_dir = in_dir / std::experimental::filesystem::path("samples/");
        std::string bloom_dir = in_dir.string() +  "/bloom_";
        size_t iteration = 1;
        create_classic_index_from_samples(samples_dir, bloom_dir + std::to_string(iteration), batch_size, num_hashes, false_positive_probability, page_size);
        while(!combine_classic_index(bloom_dir + std::to_string(iteration), bloom_dir + std::to_string(iteration + 1), batch_size)) {
            iteration++;
        }
        create_compact_index(bloom_dir + std::to_string(iteration + 1), in_dir / ("index" + isi::file::compact_index_header::file_extension), page_size);
    }

    void create(const std::experimental::filesystem::path& in_dir, std::experimental::filesystem::path out_dir,
                       size_t batch_size, size_t num_hashes, double false_positive_probability, uint64_t page_size) {
        create_folders(in_dir, out_dir / std::experimental::filesystem::path("/samples"), page_size);
        create_from_folders(out_dir, batch_size, num_hashes, false_positive_probability, page_size);
    }
}
