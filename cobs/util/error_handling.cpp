/*******************************************************************************
 * cobs/util/error_handling.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/error_handling.hpp>
#include <cstring>
#include <iostream>

namespace cobs {

void print_errno(const std::string& msg) {
    std::cerr << msg + ": " << std::strerror(errno) << std::endl;
}

void exit_error(const std::string& msg) {
    std::cerr << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

void assert_exit(bool cond, const std::string& msg) {
    if (!cond) {
        exit_error(msg);
    }
}

void exit_error_errno(const std::string& msg) {
    exit_error(msg + ": " + std::strerror(errno));
}

} // namespace cobs

/******************************************************************************/
