/*******************************************************************************
 * cobs/settings.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_SETTINGS_HEADER
#define COBS_SETTINGS_HEADER

#include <cstdlib>

namespace cobs {

//! run COBS using parallel threads, default: all cores
extern size_t gopt_threads;

} // namespace cobs

#endif // !COBS_SETTINGS_HEADER

/******************************************************************************/
