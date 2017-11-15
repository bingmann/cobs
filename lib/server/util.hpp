#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <util.hpp>
#include <cstring>

namespace isi::server {

    inline std::pair<int, uint8_t*> initialize_mmap(const std::experimental::filesystem::path& path, const stream_metadata& smd) {
        int fd = open(path.string().data(), O_RDONLY, 0);
        assert(fd != -1);

        void* mmap_ptr = mmap(NULL, smd.end_pos, PROT_READ, MAP_PRIVATE, fd, 0);
        assert(mmap_ptr != MAP_FAILED);
        if(madvise(mmap_ptr, smd.end_pos, MADV_RANDOM) != 0) {
            std::cerr << "madvise failed: " << std::strerror(errno) << std::endl;
        }
        return {fd, smd.curr_pos + reinterpret_cast<uint8_t*>(mmap_ptr)};
    }

    inline void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd) {
        if (munmap(mmap_ptr - smd.curr_pos, smd.end_pos) != 0) {
            std::cerr << "could not unmap file: " << std::strerror(errno) << std::endl;
        }
        if (close(fd) != 0) {
            std::cerr << "could not close file: " << std::strerror(errno) << std::endl;
        }
    }
}

