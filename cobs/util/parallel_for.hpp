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
#include <exception>

#include <tlx/semaphore.hpp>
#include <tlx/thread_pool.hpp>

namespace cobs {

//! thread pool singleton
extern std::unique_ptr<tlx::ThreadPool> g_thread_pool;

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
        if (!g_thread_pool)
            g_thread_pool = std::make_unique<tlx::ThreadPool>();

        tlx::Semaphore sem;
        std::atomic<size_t> counter { begin };
        std::exception_ptr eptr;
        // enqueue threads for work
        for (size_t t = 0; t < num_threads; ++t) {
            g_thread_pool->enqueue(
                [&]() {
                    try {
                        size_t i;
                        while ((i = counter++) < end) {
                            functor(i);
                        }
                    }
                    catch (...) {
                        // capture exception
                        eptr = std::current_exception();
                    }
                    // done, raise semaphore
                    sem.signal();
                });
        }
        // wait for all num_threads to finish
        sem.wait(num_threads);
        // rethrow exception
        if (eptr)
            std::rethrow_exception(eptr);
    }
}

} // namespace cobs

#endif // !COBS_UTIL_PARALLEL_FOR_HEADER

/******************************************************************************/
