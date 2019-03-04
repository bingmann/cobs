/*******************************************************************************
 * cobs/util/timer.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/timer.hpp>

#include <iomanip>
#include <iostream>

#include <tlx/die.hpp>

namespace cobs {

void Timer::active(const std::string& timer) {
    die_unless(!timer.empty());
    stop();
    if (timers.count(timer) == 0) {
        order.push_back(timer);
    }
    running = timer;
}

void Timer::stop() {
    auto new_time_point = std::chrono::high_resolution_clock::now();
    if (!running.empty()) {
        timers[running] += new_time_point - time_point;
        total_duration += new_time_point - time_point;
    }
    time_point = new_time_point;
    running = "";
}

void Timer::reset() {
    for (auto& timer : timers) {
        timer.second = std::chrono::duration<double>::zero();
    }
    total_duration = std::chrono::duration<double>::zero();
}

double Timer::get(const std::string& timer) {
    return timers.at(timer).count();
}

void Timer::print(std::ostream& ostream, size_t max_name_length, const std::string& name, const std::chrono::duration<double>& duration) const {
    ostream << std::setfill(' ') << std::left << std::setw(max_name_length) << name << " - " << duration.count() << std::right << std::endl;
}

void Timer::print(std::ostream& ostream) const {
    die_unless(running.empty());
    size_t max_name_length = total_name.size();
    for (const auto& timer : timers) {
        max_name_length = std::max(max_name_length, timer.first.size());
    }

    for (const auto& timer : order) {
        print(ostream, max_name_length, timer, timers.at(timer));
    }
    print(ostream, max_name_length, total_name, total_duration);
}

} // namespace cobs

/******************************************************************************/
