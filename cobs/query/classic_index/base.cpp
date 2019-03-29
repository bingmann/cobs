/*******************************************************************************
 * cobs/query/classic_index/base.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/classic_index/base.hpp>

#include <cobs/util/file.hpp>

namespace cobs::query::classic_index {

base::base(const fs::path& path) : query::classic_base() {
    std::ifstream ifs;
    header_ = deserialize_header<ClassicIndexHeader>(ifs, path);
    stream_pos_ = get_stream_pos(ifs);
}

uint64_t base::num_hashes() const {
    return header_.num_hashes();
}

uint64_t base::row_size() const {
    return header_.row_size();
}

uint64_t base::counts_size() const {
    return 8 * header_.row_size();
}

const std::vector<std::string>& base::file_names() const {
    return header_.file_names();
}

} // namespace cobs::query::classic_index

/******************************************************************************/
