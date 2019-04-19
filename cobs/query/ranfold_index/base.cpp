/*******************************************************************************
 * cobs/query/ranfold_index/base.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/ranfold_index/base.hpp>

#include <cobs/util/file.hpp>

namespace cobs::query::ranfold_index {

base::base(const fs::path& path) : query::Search() {
    std::ifstream ifs;
    m_header = deserialize_header<RanfoldIndexHeader>(ifs, path);
    m_smd = get_stream_pos(ifs);
}

void base::search(
    const std::string& query,
    std::vector<std::pair<uint16_t, std::string> >& result,
    double threshold, size_t num_results)
{ }

} // namespace cobs::query::ranfold_index

/******************************************************************************/
