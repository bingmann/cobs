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
#include <ostream>
#include <string>
#include <vector>

namespace cobs {

class Timer
{
private:
    //! timer entry
    struct Entry {
        uint32_t hash;
        const char* name;
        std::chrono::duration<double> duration;
    };

    //! array of timers
    std::vector<Entry> timers_;

    //! total duration
    std::chrono::duration<double> total_duration_ =
        std::chrono::duration<double>::zero();

    //! currently running timer name
    const char* running_ = nullptr;
    //! start of currently running timer name
    std::chrono::time_point<std::chrono::high_resolution_clock> time_point_;

    Entry& find_or_create(const char* name);

public:
    Timer() = default;
    void active(const char* timer);
    void stop();
    void reset();
    double get(const char* timer);
    void print(const char* info, std::ostream& os) const;
    void print(const char* info) const;

    //! add all timers from another, internally holds a global lock, because
    //! this is used to add thread values
    Timer& operator += (const Timer& b);
};

} // namespace cobs

#endif // !COBS_UTIL_TIMER_HEADER

/******************************************************************************/
