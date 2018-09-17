/*******************************************************************************
 * test/src/util.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/misc.hpp>
#include <gtest/gtest.h>
#include <stdint.h>

namespace {

void is_aligned(void* ptr, size_t alignment) {
    ASSERT_EQ((uintptr_t)ptr % alignment, 0);
}
TEST(util, allocate_aligned) {
    auto ptr1 = cobs::allocate_aligned<uint8_t>(10, cobs::get_page_size());
    is_aligned(ptr1, cobs::get_page_size());
    auto ptr2 = cobs::allocate_aligned<uint16_t>(1337, 16);
    is_aligned(ptr2, 16);
    cobs::deallocate_aligned(ptr1);
    cobs::deallocate_aligned(ptr2);
}
}

/******************************************************************************/
