#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <utility>
#include <cobs/util/serialization.hpp>
#include <unordered_map>
#include <cobs/util/fs.hpp>

namespace cobs::query {
    int open_file(const fs::path& path, int flags);
    void close_file(int fd);
    std::pair<int, uint8_t*> initialize_mmap(const fs::path& path, const stream_metadata& smd);
    void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd);
    const char* canonicalize_kmer(const char* query_8, char* kmer_raw_8, uint32_t kmer_size);
}

