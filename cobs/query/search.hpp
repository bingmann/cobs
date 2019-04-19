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

namespace cobs::query {

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
        std::vector<std::pair<uint16_t, std::string> >& result,
        double threshold = 0.0, size_t num_results = 0) = 0;

public:
    //! timer of different query phases
    Timer timer_;
};

} // namespace cobs::query

#endif // !COBS_QUERY_SEARCH_HEADER

/******************************************************************************/
