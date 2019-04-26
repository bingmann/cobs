/*******************************************************************************
 * cobs/query/ranfold_index/search_file.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/ranfold_index/search_file.hpp>

#include <cobs/util/file.hpp>

#include <tlx/unused.hpp>

namespace cobs {

RanfoldIndexSearchFile::RanfoldIndexSearchFile(const fs::path& path) {
    std::ifstream ifs;
    m_header = deserialize_header<RanfoldIndexHeader>(ifs, path);
    m_smd = get_stream_pos(ifs);
}

void RanfoldIndexSearchFile::search(
    const std::string& query,
    std::vector<std::pair<uint16_t, std::string> >& result,
    double threshold, size_t num_results)
{
    tlx::unused(query, result, threshold, num_results);
}

} // namespace cobs

/******************************************************************************/
