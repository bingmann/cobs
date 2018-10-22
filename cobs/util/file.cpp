/*******************************************************************************
 * cobs/util/file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

namespace cobs::file {

std::string file_name(const fs::path& p) {
    std::string result = p.filename().string();
    auto comp = [](char c) {
                    return c == '.';
                };
    auto iter = std::find_if(result.rbegin(), result.rend(), comp) + 1;
    iter = std::find_if(iter, result.rend(), comp) + 1;
    return result.substr(0, std::distance(iter, result.rend()));
}

} // namespace cobs::file

/******************************************************************************/
