#include <gtest/gtest.h>
#include <isi/util/misc.hpp>
#include <stdint.h>

namespace {

    void is_aligned(void* ptr, size_t alignment) {
        ASSERT_EQ((uintptr_t) ptr % alignment, 0);
    }
    TEST(util, allocate_aligned) {
        auto ptr1 = isi::allocate_aligned<uint8_t>(10, isi::get_page_size());
        is_aligned(ptr1, isi::get_page_size());
        auto ptr2 = isi::allocate_aligned<uint16_t>(1337, 16);
        is_aligned(ptr2, 16);
    }
}
