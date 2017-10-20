#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <util.hpp>

namespace genome::server {

    inline byte* initialize_mmap(const boost::filesystem::path& path, const stream_metadata& smd) {
        int fd = open(path.string().data(), O_RDONLY, 0);
        assert(fd != -1);

//  | MAP_POPULATE
        return smd.curr_pos + reinterpret_cast<byte*>(mmap(NULL, smd.end_pos, PROT_READ, MAP_PRIVATE, fd, 0));
    }
}

