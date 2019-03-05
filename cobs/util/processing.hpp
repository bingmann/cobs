/*******************************************************************************
 * cobs/util/processing.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_PROCESSING_HEADER
#define COBS_UTIL_PROCESSING_HEADER

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <vector>

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

namespace cobs {

template <typename Callback>
void get_sorted_file_names(const fs::path& in_dir,
                           std::vector<fs::path>* paths,
                           Callback callback) {
    fs::recursive_directory_iterator it(in_dir), end;
    while (it != end) {
        if (callback(*it)) {
            paths->emplace_back(*it);
        }
        ++it;
    }
    std::sort(paths->begin(), paths->end());
}

template <typename Selector, typename Callback>
bool process_file_batches(const fs::path& in_dir, const fs::path& out_dir, size_t bulk_size,
                          const std::string& file_extension_out,
                          Selector selector, Callback callback) {
    std::vector<fs::path> sorted_paths;
    get_sorted_file_names(
        in_dir, &sorted_paths, [](const fs::path&) { return true; });
    fs::create_directories(out_dir);

    std::string first_filename, last_filename;

    size_t j = 1;
    std::vector<fs::path> paths;
    for (size_t i = 0; i < sorted_paths.size(); i++) {
        if (selector(sorted_paths[i])) {
            std::string filename = cobs::base_name(sorted_paths[i]);
            if (first_filename.empty()) {
                first_filename = filename;
            }
            last_filename = filename;
            paths.push_back(sorted_paths[i]);
        }
        if (paths.size() == bulk_size || (!paths.empty() && i + 1 == sorted_paths.size())) {
            fs::path out_file = out_dir / ("[" + first_filename + "-" + last_filename + "]" + file_extension_out);
            std::cout << "BE - " << std::setfill('0') << std::setw(7) << j
                      << " - " << out_file << std::endl;
            bool exists = fs::exists(out_file);
            if (!exists || /* TODO: add command line parameter */ 1) {
                callback(paths, out_file);
            }
            std::cout << (exists ? "EX" : "OK")
                      << " - " << std::setfill('0') << std::setw(7) << j
                      << " - " << out_file << std::endl;
            paths.clear();
            first_filename.clear();
            j++;
        }
    }
    return j < 3;
}

} // namespace cobs

#endif // !COBS_UTIL_PROCESSING_HEADER

/******************************************************************************/
