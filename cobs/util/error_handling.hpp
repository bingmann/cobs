/*******************************************************************************
 * cobs/util/error_handling.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_ERROR_HANDLING_HEADER
#define COBS_UTIL_ERROR_HANDLING_HEADER
#pragma once

#include <string>

namespace cobs {

void print_errno(const std::string& msg);
void exit_error(const std::string& msg);
void assert_exit(bool cond, const std::string& msg);
void exit_error_errno(const std::string& msg);
template <class E>
void assert_throw(bool cond, const std::string& msg);

} // namespace cobs

namespace cobs {

template <class E>
void assert_throw(bool cond, const std::string& msg) {
    if (!cond) {
        throw E(msg);
    }
}

} // namespace cobs

#endif // !COBS_UTIL_ERROR_HANDLING_HEADER

/******************************************************************************/
