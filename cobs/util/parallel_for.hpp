/*******************************************************************************
 * cobs/util/parallel_for.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_PARALLEL_FOR_HEADER
#define COBS_UTIL_PARALLEL_FOR_HEADER

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace cobs {

//! run a functor in parallel
template <typename Functor>
void parallel_for(size_t begin, size_t end, size_t num_threads,
                  Functor functor) {
    if (num_threads <= 1) {
        for (size_t i = begin; i < end; ++i) {
            functor(i);
        }
    }
    else {
        std::atomic<size_t> counter { begin };
        std::vector<std::thread> threads;
        threads.resize(num_threads);
        // start threads
        for (size_t t = 0; t < num_threads; ++t) {
            threads[t] = std::thread(
                [&]() {
                    size_t i;
                    while ((i = counter++) < end) {
                        functor(i);
                    }
                });
        }
        for (size_t i = 0; i < num_threads; ++i) {
            threads[i].join();
        }
    }
}

} // namespace cobs

#endif // !COBS_UTIL_PARALLEL_FOR_HEADER

/******************************************************************************/
