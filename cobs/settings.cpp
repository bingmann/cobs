/*******************************************************************************
 * cobs/settings.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/settings.hpp>

#include <thread>

namespace cobs {

size_t gopt_threads = std::thread::hardware_concurrency();

bool gopt_load_complete_index = false;

bool gopt_disable_cache = false;

} // namespace cobs

/******************************************************************************/
