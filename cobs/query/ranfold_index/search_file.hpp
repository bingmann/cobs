/*******************************************************************************
 * cobs/query/ranfold_index/search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_RANFOLD_INDEX_SEARCH_FILE_HEADER
#define COBS_QUERY_RANFOLD_INDEX_SEARCH_FILE_HEADER

#include <cobs/file/ranfold_index_header.hpp>
#include <cobs/query/classic_search.hpp>

namespace cobs {

class RanfoldIndexSearchFile : public Search
{
protected:
    explicit RanfoldIndexSearchFile(const fs::path& path);

    RanfoldIndexHeader m_header;
    StreamPos m_smd;

public:
    virtual ~RanfoldIndexSearchFile() = default;

    void search(
        const std::string& query,
        std::vector<std::pair<uint16_t, std::string> >& result,
        double threshold, size_t num_results = 0) final;
};

} // namespace cobs

#endif // !COBS_QUERY_RANFOLD_INDEX_SEARCH_FILE_HEADER

/******************************************************************************/
