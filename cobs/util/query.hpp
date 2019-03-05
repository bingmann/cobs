/*******************************************************************************
 * cobs/util/query.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_QUERY_HEADER
#define COBS_UTIL_QUERY_HEADER

#include <cobs/util/fs.hpp>
#include <cobs/util/serialization.hpp>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <utility>

namespace cobs {

int open_file(const fs::path& path, int flags);
void close_file(int fd);
std::pair<int, uint8_t*> initialize_mmap(const fs::path& path, const StreamMetadata& smd);
void destroy_mmap(int fd, uint8_t* mmap_ptr, const StreamMetadata& smd);
const char * canonicalize_kmer(const char* query_8, char* kmer_buffer, uint32_t kmer_size);

} // namespace cobs

#endif // !COBS_UTIL_QUERY_HEADER

/******************************************************************************/
