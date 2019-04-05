/*******************************************************************************
 * cobs/query/base.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_BASE_HEADER
#define COBS_QUERY_BASE_HEADER

#include <cobs/util/timer.hpp>

#include <tlx/logger.hpp>

namespace cobs::query {

class base
{
public:
    virtual ~base() = default;
    Timer& get_timer() { return timer_; }

    virtual void search(const std::string& query,
                        std::vector<std::pair<uint16_t, std::string> >& result,
                        size_t num_results = 0) = 0;

protected:
    Timer timer_;
};

} // namespace cobs::query

#endif // !COBS_QUERY_BASE_HEADER

/******************************************************************************/
