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
    m_header = file::deserialize_header<ClassicIndexHeader>(ifs, path);
    m_smd = get_stream_metadata(ifs);
}

uint64_t base::num_hashes() const {
    return m_header.num_hashes();
}

uint64_t base::block_size() const {
    return m_header.block_size();
}

uint64_t base::counts_size() const {
    return 8 * m_header.block_size();
}

const std::vector<std::string>& base::file_names() const {
    return m_header.file_names();
}

} // namespace cobs::query::classic_index

/******************************************************************************/
