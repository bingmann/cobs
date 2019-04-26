/*******************************************************************************
 * cobs/query/classic_index/search_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/classic_index/search_file.hpp>

#include <cobs/util/file.hpp>

namespace cobs {

ClassicIndexSearchFile::ClassicIndexSearchFile(const fs::path& path) {
    std::ifstream ifs;
    header_ = deserialize_header<ClassicIndexHeader>(ifs, path);
    stream_pos_ = get_stream_pos(ifs);
}

uint64_t ClassicIndexSearchFile::counts_size() const {
    return 8 * header_.row_size();
}

} // namespace cobs

/******************************************************************************/
