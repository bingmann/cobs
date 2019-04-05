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
#include <tlx/logger.hpp>
#include <tlx/string/hash_djb2.hpp>

namespace cobs {

Timer::Entry& Timer::find_or_create(const char* name) {
    uint32_t h = tlx::hash_djb2(name);
    for (size_t i = 0; i < timers_.size(); ++i) {
        if (timers_[i].hash == h && strcmp(timers_[i].name, name) == 0)
            return timers_[i];
    }
    Entry new_entry;
    new_entry.hash = h;
    new_entry.name = name;
    new_entry.duration = std::chrono::duration<double>::zero();
    timers_.emplace_back(new_entry);
    return timers_.back();
}

void Timer::active(const char* timer) {
    die_unless(timer);
    // yes, compare string pointers, not contents
    if (running_ == timer) {
        LOG1 << "Timer: starting same timer twice, maybe multi-threading?";
    }
    stop();
    running_ = timer;
}

void Timer::stop() {
    auto new_time_point = std::chrono::high_resolution_clock::now();
    if (running_) {
        Entry& e = find_or_create(running_);
        e.duration += new_time_point - time_point_;
        total_duration_ += new_time_point - time_point_;
    }
    time_point_ = new_time_point;
    running_ = nullptr;
}

void Timer::reset() {
    timers_.clear();
    total_duration_ = std::chrono::duration<double>::zero();
}

double Timer::get(const char* name) {
    return find_or_create(name).duration.count();
}

void Timer::print(std::ostream& os, size_t max_name_length,
                  const char* name,
                  const std::chrono::duration<double>& duration) const {
    os << std::setfill(' ') << std::left << std::setw(max_name_length)
       << name << " - " << duration.count() << std::right << std::endl;
}

Timer& Timer::operator += (const Timer& b) {
    for (const Entry& t : b.timers_) {
        Entry& e = find_or_create(t.name);
        e.duration += t.duration;
    }
    total_duration_ += b.total_duration_;
    return *this;
}

void Timer::print(std::ostream& os) const {
    die_unless(!running_);
    static const char* total_name = "total";

    size_t max_name_length = std::strlen(total_name);
    for (const Entry& timer : timers_) {
        max_name_length = std::max(max_name_length, std::strlen(timer.name));
    }
    for (const Entry& timer : timers_) {
        print(os, max_name_length, timer.name, timer.duration);
    }
    print(os, max_name_length, total_name, total_duration_);
}

std::ostream& operator << (std::ostream& os, const Timer& t) {
    t.print(os);
    return os;
}

} // namespace cobs

/******************************************************************************/
