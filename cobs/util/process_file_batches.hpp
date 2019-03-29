/*******************************************************************************
 * cobs/util/process_file_batches.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_PROCESS_FILE_BATCHES_HEADER
#define COBS_UTIL_PROCESS_FILE_BATCHES_HEADER

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

#include <tlx/logger.hpp>

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
size_t process_file_batches(const fs::path& in_dir, const fs::path& out_dir,
                            size_t batch_size,
                            Selector selector, Callback callback) {
    std::vector<fs::path> sorted_paths;
    get_sorted_file_names(
        in_dir, &sorted_paths, [](const fs::path&) { return true; });
    fs::create_directories(out_dir);

    std::string first_filename, last_filename;

    size_t batch_num = 0;
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
        if (paths.size() == batch_size ||
            (!paths.empty() && i + 1 == sorted_paths.size()))
        {
            std::string out_file =
                pad_index(batch_num) + '_' +
                '[' + first_filename + '-' + last_filename + ']';

            LOG1 << "IN - " << out_file;
            callback(paths, out_file);
            LOG1 << "OK - " << out_file;

            paths.clear();
            first_filename.clear();
            batch_num++;
        }
    }
    return batch_num;
}

} // namespace cobs

#endif // !COBS_UTIL_PROCESS_FILE_BATCHES_HEADER

/******************************************************************************/