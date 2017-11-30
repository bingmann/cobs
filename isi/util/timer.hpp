#pragma once

#include <chrono>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <isi/util/misc.hpp>

namespace isi {
    class timer {
    private:
        std::unordered_map<std::string, std::chrono::duration<double>> timers;
        std::vector<std::string> order;
        std::string total_name = "total";
        std::chrono::duration<double> total_duration = std::chrono::duration<double>::zero();
        std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
        std::string running;

        void print(std::ostream& ostream, size_t max_name_length, const std::string& name, const std::chrono::duration<double>& duration) const;

    public:
        void active(const std::string& timer);
        void stop();
        void reset();
        void print(std::ostream& ostream) const;
    };
}
