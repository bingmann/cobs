/*******************************************************************************
 * cobs/util/timer.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_TIMER_HEADER
#define COBS_UTIL_TIMER_HEADER

#include <chrono>
#include <cobs/util/misc.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace cobs {

class Timer
{
private:
    std::unordered_map<std::string, std::chrono::duration<double> > timers;
    std::vector<std::string> order;
    std::string total_name = "total";
    std::chrono::duration<double> total_duration = std::chrono::duration<double>::zero();
    std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
    std::string running;

    void print(std::ostream& ostream, size_t max_name_length, const std::string& name, const std::chrono::duration<double>& duration) const;

public:
    Timer() = default;
    void active(const std::string& timer);
    void stop();
    void reset();
    double get(const std::string& timer);
    void print(std::ostream& ostream) const;
};

} // namespace cobs

#endif // !COBS_UTIL_TIMER_HEADER

/******************************************************************************/
