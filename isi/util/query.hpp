#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <experimental/filesystem>
#include <cstring>
#include <utility>
#include <isi/util/serialization.hpp>

namespace isi::query {
    std::pair<int, uint8_t*> initialize_mmap(const std::experimental::filesystem::path& path, const stream_metadata& smd);
    void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd);
}

