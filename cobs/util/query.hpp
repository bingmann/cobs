/*******************************************************************************
 * cobs/util/query.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_QUERY_HEADER
#define COBS_UTIL_QUERY_HEADER

#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <utility>

#include <cobs/util/fs.hpp>
#include <cobs/util/serialization.hpp>

namespace cobs {

int open_file(const fs::path& path, int flags);
void close_file(int fd);

struct MMapHandle {
    int fd;
    uint8_t* data;
    uint64_t size;
};

MMapHandle initialize_mmap(const fs::path& path);
void destroy_mmap(MMapHandle& handle);

//! Canonicalize a k-mer. Given
bool canonicalize_kmer(const char* input, char* output, size_t size);

} // namespace cobs

#endif // !COBS_UTIL_QUERY_HEADER

/******************************************************************************/
