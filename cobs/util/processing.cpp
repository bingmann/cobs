/*******************************************************************************
 * cobs/util/processing.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/fs.hpp>
#include <cobs/util/processing.hpp>

namespace cobs {

void get_sorted_file_names(const fs::path& in_dir,
                           const fs::path& out_dir,
                           std::vector<fs::path>& paths) {
    fs::create_directories(out_dir);
    fs::recursive_directory_iterator it(in_dir), end;
    std::copy(it, end, std::back_inserter(paths));
    std::sort(paths.begin(), paths.end());
}

} // namespace cobs

/******************************************************************************/
