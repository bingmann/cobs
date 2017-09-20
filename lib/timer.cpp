//
// Created by Florian Gauger on 18.09.2017.
//

#include "timer.hpp"

#include <iostream>
#include <iomanip>

timer::timer(std::initializer_list<std::string> measurement_names) {
    for (const auto& measurement_name: measurement_names) {
        measurements.emplace_back(std::make_pair(measurement_name, std::chrono::duration<double>()));
        max_name_length = std::max(max_name_length, measurement_name.size());
    }
}

void timer::start() {
    if (running) {
        throw std::logic_error("timer has already started");
    }
    running = true;
    time_point = std::chrono::high_resolution_clock::now();
}

void timer::end() {
    if (!running) {
        throw std::logic_error("timer has not been started");
    }
    next();
    running = false;
    position = 0;
}

void timer::next() {
    if (!running) {
        throw std::logic_error("timer has not been started");
    } else if (position == measurements.size()) {
        throw std::logic_error("all measurements have been taken");
    }
    auto new_time_point = std::chrono::high_resolution_clock::now();
    measurements[position].second += new_time_point - time_point;
    total_duration += new_time_point - time_point;
    time_point = new_time_point;
    position++;
}

void timer::reset() {
    running = false;
    position = 0;
    for (auto& measurement : measurements) {
        measurement.second = std::chrono::duration<double>::zero();
    }
    total_duration = std::chrono::duration<double>::zero();
}

void timer::print(std::ostream &ostream, const std::string& name, std::chrono::duration<double> duration) const {
    ostream << std::left << std::setw(max_name_length) << name << " - " << duration.count() << std::endl;
}

void timer::print(std::ostream &ostream) const {
    for (const auto& measurement: measurements) {
        print(ostream, measurement.first, measurement.second);
    }
    print(ostream, total_name, total_duration);
}























