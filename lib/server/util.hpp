#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <util.hpp>
#include <cstring>

namespace genome::server {

    inline byte* initialize_mmap(const std::experimental::filesystem::path& path, const stream_metadata& smd) {
        int fd = open(path.string().data(), O_RDONLY, 0);
        assert(fd != -1);

        void* mmap_ptr = mmap(NULL, smd.end_pos, PROT_READ, MAP_PRIVATE, fd, 0);
        if(madvise(mmap_ptr, smd.end_pos - smd.curr_pos, MADV_RANDOM) != 0) {
            std::cerr << "madvise failed: " << std::strerror(errno) << std::endl;
        }
        return smd.curr_pos + reinterpret_cast<byte*>(mmap_ptr);
    }
}

