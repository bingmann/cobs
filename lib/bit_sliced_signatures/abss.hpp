#pragma once


#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <file/sample_header.hpp>
#include <file/util.hpp>
#include <file/abss_header.hpp>
#include <cmath>
#include <bit_sliced_signatures/bss.hpp>

namespace genome::abss {
    void create_folders(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir, size_t batch_size) {
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
            if (i % batch_size == 0) {
                sub_out_dir = out_dir.string() + "/samples/" + std::to_string(batch);
                boost::filesystem::create_directories(sub_out_dir);
                batch++;
            }
            boost::filesystem::rename(p, sub_out_dir / p.filename());
            i++;
        }
    }

    double calc_signature_size_ratio(double num_hashes, double false_positive_probability) {
        double denominator = std::log(1 - std::pow(false_positive_probability, 1 / num_hashes));
        double result = -num_hashes / denominator;
        assert(result > 0);
        return result;
    }

    uint64_t calc_signature_size(size_t num_elements, double num_hashes, double false_positive_probability) {
        double signature_size_ratio = calc_signature_size_ratio(num_hashes, false_positive_probability);
        double result = std::ceil(num_elements * signature_size_ratio);
        assert(result <= UINT64_MAX);
        return result;
    }

    void create_bsss_from_samples(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir,
                                           size_t batch_size, size_t num_hashes, double false_positive_probability) {
        assert(batch_size % 8 == 0);
        boost::system::error_code ec;
        for (boost::filesystem::directory_iterator it(in_dir), end; it != end; it.increment(ec)) {
            boost::filesystem::path p = it->path();
            if(boost::filesystem::is_directory(p)) {
                size_t max_file_size = 0;
                for (boost::filesystem::directory_iterator sub_it(p); sub_it != end; sub_it.increment(ec)) {
                    if (sub_it->path().extension() == genome::file::sample_header::file_extension) {
                        max_file_size = std::max(max_file_size, boost::filesystem::file_size(sub_it->path()));
                    }
                }

                size_t signature_size = calc_signature_size(max_file_size / 8, num_hashes, false_positive_probability);
                bss::create_from_samples(p, out_dir / p.filename(), signature_size, batch_size / 8, num_hashes);
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

    void create_abss(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_file) {
        std::vector<boost::filesystem::path> paths;
        boost::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return boost::filesystem::is_regular_file(p) && p.path().extension() == genome::file::bss_header::file_extension;
        });
        std::sort(paths.begin(), paths.end(), [](const auto& p1, const auto& p2) {
            return boost::filesystem::file_size(p1) < boost::filesystem::file_size(p2);
        });


        uint32_t page_size = getpagesize();
        assert(page_size == 4096); //todo just in case

        std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> parameters;
        std::vector<std::string> file_names;
        for (const auto& p: paths) {
            std::ifstream ifs;
            auto bssh = genome::file::deserialize_header<genome::file::bss_header>(ifs, p);
            parameters.emplace_back(std::make_tuple(bssh.signature_size(), bssh.block_size(), bssh.num_hashes()));
            file_names.insert(file_names.end(), bssh.file_names().begin(), bssh.file_names().end());
//            assert(bssh.block_size() == page_size);
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

    void create_abss_from_samples(const boost::filesystem::path& in_dir, size_t processing_batch_size, size_t num_hashes, double false_positive_probability) {
        boost::filesystem::path samples_dir = in_dir / "samples/";
        std::string bloom_dir = in_dir.string() +  "/bloom_";
        size_t iteration = 1;
        create_bsss_from_samples(samples_dir, bloom_dir + std::to_string(iteration), processing_batch_size, num_hashes, false_positive_probability);
        while(!combine_bss(bloom_dir + std::to_string(iteration), bloom_dir + std::to_string(iteration + 1), processing_batch_size)) {
            iteration++;
        }
        create_abss(bloom_dir + std::to_string(iteration + 1), in_dir / ("filter" + genome::file::abss_header::file_extension));
    }
}
