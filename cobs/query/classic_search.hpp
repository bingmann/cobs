/*******************************************************************************
 * cobs/query/classic_search.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_SEARCH_HEADER
#define COBS_QUERY_CLASSIC_SEARCH_HEADER

#include <cobs/query/index_file.hpp>
#include <cobs/query/search.hpp>
#include <cobs/util/query.hpp>

namespace cobs {

class ClassicSearch : public Search
{
public:
    //! method to try to auto-detect and load IndexSearchFile
    ClassicSearch(std::string path);

    ClassicSearch(std::shared_ptr<IndexSearchFile> index);

    ClassicSearch(std::vector<std::shared_ptr<IndexSearchFile> > indices);

    size_t num_documents() const;

    void search(
        const std::string& query,
        std::vector<SearchResult>& result,
        double threshold = 0.0, size_t num_results = 0) final;

protected:
    //! reference to index file query object to retrieve data
    std::vector<std::shared_ptr<IndexSearchFile> > index_files_;
};

/*----------------------------------------------------------------------------*/
// hacky variables to disable expansion table variants for better testing

//! disable 8-bit expansion table
extern bool classic_search_disable_8bit;
//! disable 16-bit expansion table
extern bool classic_search_disable_16bit;
//! disable 32-bit expansion table
extern bool classic_search_disable_32bit;

//! disable SSE2 versions of expansion
extern bool classic_search_disable_sse2;

/*----------------------------------------------------------------------------*/

} // namespace cobs

#endif // !COBS_QUERY_CLASSIC_SEARCH_HEADER

/******************************************************************************/
