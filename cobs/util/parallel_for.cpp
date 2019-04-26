/*******************************************************************************
 * cobs/util/parallel_for.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/parallel_for.hpp>

namespace cobs {

//! thread pool singleton
std::unique_ptr<tlx::ThreadPool> g_thread_pool;

} // namespace cobs

/******************************************************************************/
