/*******************************************************************************
 * cobs/util/misc.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/misc.hpp>

#include <unistd.h>

#include <tlx/die.hpp>

namespace cobs {

uint64_t get_page_size() {
    int page_size = getpagesize();
    die_unless(page_size > 0);
    die_unless(page_size == 4096);     // todo check for experiments
    return (uint64_t)page_size;
}

uint64_t get_memory_size() {
    return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
}

uint64_t get_memory_size(size_t percentage) {
    return get_memory_size() * percentage / 100;
}

std::string random_sequence(size_t size, size_t seed) {
    std::default_random_engine rng(seed);
    return random_sequence_rng(size, rng);
}

} // namespace cobs

/******************************************************************************/
