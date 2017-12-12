#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <experimental/filesystem>
#include <cstring>
#include <utility>
#include <isi/util/serialization.hpp>
#include <unordered_map>

namespace isi::query {
    int open_file(const std::experimental::filesystem::path& path);
    void close_file(int fd);
    std::pair<int, uint8_t*> initialize_mmap(const std::experimental::filesystem::path& path, const stream_metadata& smd);
    void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd);
    const char* canonicalize_kmer(const char* query_8, char* kmer_raw_8, uint32_t kmer_size);
}

