/*******************************************************************************
 * cobs/query/search.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_SEARCH_HEADER
#define COBS_QUERY_SEARCH_HEADER

#include <cobs/util/timer.hpp>

namespace cobs {

struct SearchResult {
    //! string reference to document name
    const char* doc_name;
    //! score (number of matched k-mers)
    uint32_t score;

    SearchResult() = default;

    SearchResult(const char* doc_name, uint32_t score)
        : doc_name(doc_name), score(score) { }
};

class Search
{
public:
    virtual ~Search() = default;

    //! Returns timer_
    Timer& timer() { return timer_; }
    //! Returns timer_
    const Timer& timer() const { return timer_; }

    virtual void search(
        const std::string& query,
        std::vector<SearchResult>& result,
        double threshold = 0.0, size_t num_results = 0) = 0;

public:
    //! timer of different query phases
    Timer timer_;
};

} // namespace cobs

#endif // !COBS_QUERY_SEARCH_HEADER

/******************************************************************************/
