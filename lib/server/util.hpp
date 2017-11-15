#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <util.hpp>
#include <cstring>

namespace isi::server {
    inline std::pair<int, uint8_t*> initialize_mmap(const std::experimental::filesystem::path& path, const stream_metadata& smd) {
        int fd = open(path.string().data(), O_RDONLY, 0);
        if(fd == -1) {
            exit_error_errno("could not open index file " + path.string());
        }

        void* mmap_ptr = mmap(NULL, smd.end_pos, PROT_READ, MAP_PRIVATE, fd, 0);
        if(mmap_ptr == MAP_FAILED) {
            exit_error_errno("mmap failed");
        }
        if(madvise(mmap_ptr, smd.end_pos, MADV_RANDOM)) {
            print_errno("madvise failed");
        }
        return {fd, smd.curr_pos + reinterpret_cast<uint8_t*>(mmap_ptr)};
    }

    inline void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd) {
        if (munmap(mmap_ptr - smd.curr_pos, smd.end_pos)) {
            print_errno("could not unmap index file");
        }
        if (close(fd)) {
            print_errno("could not close index file");
        }
    }
}

