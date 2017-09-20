//
// Created by Florian Gauger on 18.09.2017.
//

#pragma once


#include <chrono>
#include <string>
#include <tuple>
#include <vector>

class timer {
private:
    std::vector<std::pair<std::string, std::chrono::duration<double>>> measurements;
    std::string total_name = "total";
    std::chrono::duration<double> total_duration;
    std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
    size_t position = 0;
    bool running = false;
    size_t max_name_length = total_name.size();
    void print(std::ostream &ostream, const std::string &name, std::chrono::duration<double> duration) const;
public:
    timer(std::initializer_list<std::string> values);
    void start();
    void end();
    void next();
    void reset();
    void print(std::ostream& ostream) const;
};



