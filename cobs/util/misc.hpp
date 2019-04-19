/*******************************************************************************
 * cobs/util/misc.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_MISC_HEADER
#define COBS_UTIL_MISC_HEADER

#include <array>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <random>

#include <xxhash.h>

#include <tlx/string/ssprintf.hpp>

namespace cobs {

uint64_t get_page_size();

uint64_t get_memory_size();

uint64_t get_memory_size(size_t percentage);

template <typename RandomGenerator>
std::string random_sequence_rng(size_t size, RandomGenerator& rng) {
    static const std::array<char, 4> basepairs = { 'A', 'C', 'G', 'T' };
    std::string result;
    for (size_t i = 0; i < size; i++) {
        result += basepairs[rng() % 4];
    }
    return result;
}

std::string random_sequence(size_t size, size_t seed);

template <typename T>
T * allocate_aligned(uint64_t size, size_t alignment) {
    T* ptr;
    int r = posix_memalign(reinterpret_cast<void**>(&ptr), alignment, sizeof(T) * size);
    if (r != 0)
        throw std::runtime_error("Out of memory");
    std::fill(ptr, ptr + size, 0);
    return ptr;
}

static inline
void deallocate_aligned(void* ptr) {
    free(ptr);
}

static inline
std::string pad_index(unsigned index, int size = 6) {
    return tlx::ssprintf("%0*u", size, index);
}

/*!
 * Constructs the hash used by the signatures.
 */
template <typename Callback>
void process_hashes(const void* input, size_t size, uint64_t signature_size,
                    uint64_t num_hashes, Callback callback) {
    for (unsigned int i = 0; i < num_hashes; i++) {
        uint64_t hash = XXH64(input, size, i);
        callback(hash % signature_size);
    }
}

} // namespace cobs

#endif // !COBS_UTIL_MISC_HEADER

/******************************************************************************/
