#pragma once


#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <file/sample_header.hpp>
#include <file/util.hpp>
#include <file/abss_header.hpp>
#include <bit_sliced_signatures/bss.hpp>
#include <boost/utility.hpp>

namespace genome::abss {
    void create_folders(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir, uint64_t page_size = get_page_size()) {
        std::vector<boost::filesystem::path> paths;
        boost::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return boost::filesystem::is_regular_file(p) && p.path().extension() == genome::file::sample_header::file_extension;
        });
        std::sort(paths.begin(), paths.end(), [](const auto& p1, const auto& p2) {
            return boost::filesystem::file_size(p1) < boost::filesystem::file_size(p2);
        });

        size_t batch = 1;
        size_t i = 0;
        boost::filesystem::path sub_out_dir;
        for(const auto& p: paths) {
            if (i % (8 * page_size) == 0) {
                sub_out_dir = out_dir.string() + "/samples/" + std::to_string(batch);
                boost::filesystem::create_directories(sub_out_dir);
                batch++;
            }
            boost::filesystem::rename(p, sub_out_dir / p.filename());
            i++;
        }
    }

    void create_bss_from_samples(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir, size_t batch_size,
                                 size_t num_hashes, double false_positive_probability, uint64_t page_size) {
        assert(batch_size % 8 == 0);
        std::vector<boost::filesystem::path> paths;
        std::copy_if(boost::filesystem::directory_iterator(in_dir), boost::filesystem::directory_iterator(), std::back_inserter(paths), [](const auto& p) {
            return boost::filesystem::is_directory(p);
        });
        std::sort(paths.begin(), paths.end());

        boost::system::error_code ec;
        for (const auto& p: paths) {
            size_t num_files = 0;
            size_t max_file_size = 0;
            for (boost::filesystem::directory_iterator sub_it(p), end; sub_it != end; sub_it.increment(ec)) {
                if (sub_it->path().extension() == genome::file::sample_header::file_extension) {
                    max_file_size = std::max(max_file_size, boost::filesystem::file_size(sub_it->path()));
                    num_files++;
                }
            }
            size_t signature_size = calc_signature_size(max_file_size / 8, num_hashes, false_positive_probability);
            bss::create_from_samples(p, out_dir / p.filename(), signature_size, batch_size / 8, num_hashes);

            if (num_files != 8 * page_size) {
                assert(num_files < 8 * page_size);
                assert(p == paths.back());
                genome::bss bss(signature_size, (8 * page_size - num_files) / 8, num_hashes);
                genome::file::serialize(out_dir / p.filename() / ("padding" + file::bss_header::file_extension), bss, std::vector<std::string>());
            }
        }
    }

    bool combine_bss(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir, size_t batch_size) {
        bool all_combined = false;
        boost::system::error_code ec;
        for (boost::filesystem::directory_iterator it(in_dir), end; it != end; it.increment(ec)) {
            if (boost::filesystem::is_directory(it->path())) {
                size_t signature_size = 0;
                size_t num_hashes = 0;
                for (boost::filesystem::directory_iterator bloom_it(it->path());
                     bloom_it != end; bloom_it.increment(ec)) {
                    if (bloom_it->path().extension() == genome::file::bss_header::file_extension) {
                        std::ifstream ifs;
                        auto bssh = file::deserialize_header<file::bss_header>(ifs, bloom_it->path());
                        signature_size = bssh.signature_size();
                        num_hashes = bssh.num_hashes();
                        break;
                    }
                }
                if (signature_size != 0) {
                    all_combined = genome::bss::combine_bss(in_dir / it->path().filename(), out_dir / it->path().filename(),
                                                            signature_size, num_hashes,
                                                            batch_size);
                } else {
                    std::cerr << "empty directory" << std::endl;
                }
            }
        }
        return all_combined;
    }

    void create_abss(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_file, uint64_t page_size) {
        std::vector<boost::filesystem::path> paths;
        boost::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return boost::filesystem::is_regular_file(p) && p.path().extension() == genome::file::bss_header::file_extension;
        });
        std::sort(paths.begin(), paths.end(), [](const auto& p1, const auto& p2) {
            return boost::filesystem::file_size(p1) < boost::filesystem::file_size(p2);
        });

        std::vector<genome::file::abss_header::parameter> parameters;
        std::vector<std::string> file_names;
        for (const auto& p: paths) {
            std::ifstream ifs;
            auto bssh = genome::file::deserialize_header<genome::file::bss_header>(ifs, p);
            parameters.push_back({bssh.signature_size(), bssh.num_hashes()});
            file_names.insert(file_names.end(), bssh.file_names().begin(), bssh.file_names().end());
            assert(bssh.block_size() == page_size);
        }

        genome::file::abss_header abssh(parameters, file_names, page_size);
        std::ofstream ofs;
        genome::file::serialize_header(ofs, out_file, abssh);

        std::vector<char> buffer(32 * page_size);
        for (const auto& p: paths) {
            std::ifstream ifs;
            genome::file::deserialize_header<genome::file::bss_header>(ifs, p);
            genome::stream_metadata smd = get_stream_metadata(ifs);
            size_t data_size = smd.end_pos - smd.curr_pos;
            while(data_size > 0) {
                size_t num_bytes = std::min(buffer.size(), data_size);
                ifs.read(buffer.data(), num_bytes);
                data_size -= num_bytes;
                ofs.write(buffer.data(), num_bytes);
            }
        }
    }

    void create_abss_from_samples(const boost::filesystem::path& in_dir, size_t batch_size, size_t num_hashes,
                                  double false_positive_probability, uint64_t page_size = get_page_size()) {
        boost::filesystem::path samples_dir = in_dir / "samples/";
        std::string bloom_dir = in_dir.string() +  "/bloom_";
        size_t iteration = 1;
        create_bss_from_samples(samples_dir, bloom_dir + std::to_string(iteration), batch_size, num_hashes, false_positive_probability, page_size);
        while(!combine_bss(bloom_dir + std::to_string(iteration), bloom_dir + std::to_string(iteration + 1), batch_size)) {
            iteration++;
        }
        create_abss(bloom_dir + std::to_string(iteration + 1), in_dir / ("filter" + genome::file::abss_header::file_extension), page_size);
    }
}
