#include <isi/util/timer.hpp>

#include <cassert>
#include <iostream>
#include <iomanip>

namespace isi {
    void timer::active(const std::string& timer) {
        assert(!timer.empty());
        stop();
        if (timers.count(timer) == 0) {
            order.push_back(timer);
        }
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

    void timer::print(std::ostream& ostream, size_t max_name_length, const std::string& name, const std::chrono::duration<double>& duration) const {
        ostream << std::setfill(' ') << std::left << std::setw(max_name_length) << name << " - " << duration.count() << std::right << std::endl;
    }

    void timer::print(std::ostream& ostream) const {
        assert(running.empty());
        size_t max_name_length = total_name.size();
        for (const auto&timer: timers) {
            max_name_length = std::max(max_name_length, timer.first.size());
        }

        for (const auto& timer: order) {
            print(ostream, max_name_length, timer, timers.at(timer));
        }
        print(ostream, max_name_length, total_name, total_duration);
    }
}
