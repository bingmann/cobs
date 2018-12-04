/*******************************************************************************
 * test/src/addressable_priority_queue_test.cpp
 *
 * Copyright (c) 2018 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/addressable_priority_queue.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <gtest/gtest.h>

namespace cobs {

// instantiations
template class AddressablePriorityQueue<std::pair<size_t, size_t>, size_t>;

} // namespace cobs

/******************************************************************************/
// AddressablePriorityQueue

TEST(AddressablePriorityQueue, simple) {
    cobs::AddressablePriorityQueue<size_t, size_t> apq;

    // insert some values
    for (size_t i = 1; i <= 128; ++i) {
        apq.insert(i, /* priority */ 128 - i);
    }

    die_unequal(apq.size(), 128u);

    // empty out half
    for (size_t i = 0; i < 64; ++i) {
        die_unequal(apq.top(), 128u - i);
        die_unequal(apq.top_priority(), i);
        apq.pop();
    }

    die_unequal(apq.size(), 64u);

    // update half of remaining values
    for (size_t i = 1; i <= 32; ++i) {
        auto r = apq.insert(i, /* new priority */ i);
        die_unless(!r.second);
    }

    die_unequal(apq.size(), 64u);

    // so we first get 1..32, then 64..33
    for (size_t i = 1; i <= 32; ++i) {
        die_unequal(apq.top(), i);
        die_unequal(apq.top_priority(), i);
        apq.pop();
    }
    for (size_t i = 64; i >= 33; --i) {
        die_unequal(apq.top(), i);
        die_unequal(apq.top_priority(), 128 - i);
        apq.pop();
    }

    die_unless(apq.empty());
}

/******************************************************************************/
