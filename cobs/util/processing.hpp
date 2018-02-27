#pragma once

#include <vector>
#include <iostream>
#include <experimental/filesystem>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <functional>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <cstring>

#include <cobs/util/file.hpp>

namespace cobs {
    void get_sorted_file_names(const std::experimental::filesystem::path& in_dir,
                               const std::experimental::filesystem::path& out_dir,
                               std::vector<std::experimental::filesystem::path>& paths);

    template<class T, typename Functor>
    bool bulk_process_files(const std::experimental::filesystem::path& in_dir,
                            const std::experimental::filesystem::path& out_dir, size_t bulk_size,
                            const std::string& file_extension_out, const Functor& callback);
};


namespace cobs {
    template<class T, typename Functor>
    bool bulk_process_files(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, size_t bulk_size,
                            const std::string& file_extension_out, const Functor& callback) {
        std::vector<std::experimental::filesystem::path> sorted_paths;
        get_sorted_file_names(in_dir, out_dir, sorted_paths);

        std::string first_filename;
        std::string last_filename;

        size_t j = 1;
        std::vector<std::experimental::filesystem::path> paths;
        for (size_t i = 0; i < sorted_paths.size(); i++) {
            if (cobs::file::file_is<T>(sorted_paths[i])) {
                std::string filename = cobs::file::file_name(sorted_paths[i]);
                if (first_filename.empty()) {
                    first_filename = filename;
                }
                last_filename = filename;
                paths.push_back(sorted_paths[i]);
            }
            if (paths.size() == bulk_size || (!paths.empty() && i + 1 == sorted_paths.size())) {
                std::experimental::filesystem::path out_file = out_dir / ("[" + first_filename + "-" + last_filename + "]" + file_extension_out);
                std::cout << "BE - " << std::setfill('0') << std::setw(7) << j << " - " << out_file << std::flush;
                bool exists = std::experimental::filesystem::exists(out_file);
                if (!exists) {
                    callback(paths, out_file);
                }
                std::cout << "\r" << (exists ? "EX" : "OK") << " - " << std::setfill('0') << std::setw(7) << j << " - " << out_file << std::endl;
                paths.clear();
                first_filename.clear();
                j++;
            }
        }
        return j < 3;
    }
}
