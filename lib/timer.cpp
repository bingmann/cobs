#include "timer.hpp"

#include <iostream>
#include <iomanip>

namespace genome {
    void timer::active(const std::string& timer) {
        stop();
        running = timer;
    }

    void timer::stop() {
        auto new_time_point = std::chrono::high_resolution_clock::now();
        if (!running.empty()) {
            timers[running] += new_time_point - time_point;
            total_duration += new_time_point - time_point;
        }
        time_point = new_time_point;
        running = "";
    }

    void timer::reset() {
        for (auto& timer : timers) {
            timer.second = std::chrono::duration<double>::zero();
        }
        total_duration = std::chrono::duration<double>::zero();
    }

    void timer::print(std::ostream& ostream, size_t max_name_length, const std::string& name, std::chrono::duration<double> duration) const {
        ostream << std::left << std::setw(max_name_length) << name << " - " << duration.count() << std::endl;
    }

    void timer::print(std::ostream& ostream) const {
        size_t max_name_length = total_name.size();
        for (const auto&timer: timers) {
            max_name_length = std::max(max_name_length, timer.first.size());

        }

        for (const auto& timer: timers) {
            print(ostream, max_name_length, timer.first, timer.second);
        }
        print(ostream, max_name_length, total_name, total_duration);
    }
}
