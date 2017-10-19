#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>

namespace genome::server {
    const uint64_t m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                   281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                   281479271743488, 281479271743489};

    inline byte* initialize_mmap(const boost::filesystem::path& path, std::ifstream& ifs) {
        stream_metadata smd = get_stream_metadata(ifs);
        int fd = open(path.string().data(), O_RDONLY, 0);
        assert(fd != -1);

//  | MAP_POPULATE
        return smd.curr_pos + reinterpret_cast<byte*>(mmap(NULL, smd.end_pos, PROT_READ, MAP_PRIVATE, fd, 0));
    }
}

