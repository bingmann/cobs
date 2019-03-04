/*******************************************************************************
 * cobs/util/query.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <algorithm>
#include <cobs/util/error_handling.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/query.hpp>
#include <iostream>
#include <unistd.h>

namespace cobs::query {

int open_file(const fs::path& path, int flags) {
    int fd = open(path.string().data(), flags, 0);
    if (fd == -1) {
        exit_error_errno("could not open index file " + path.string());
    }
    return fd;
}

void close_file(int fd) {
    if (close(fd)) {
        print_errno("could not close index file");
    }
}

std::pair<int, uint8_t*> initialize_mmap(const fs::path& path, const stream_metadata& smd) {
    int fd = open_file(path, O_RDONLY);
    void* mmap_ptr = mmap(nullptr, smd.end_pos, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmap_ptr == MAP_FAILED) {
        exit_error_errno("mmap failed");
    }
    if (madvise(mmap_ptr, smd.end_pos, MADV_RANDOM)) {
        print_errno("madvise failed");
    }
    return {
               fd, smd.curr_pos + reinterpret_cast<uint8_t*>(mmap_ptr)
    };
}

void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd) {
    if (munmap(mmap_ptr - smd.curr_pos, smd.end_pos)) {
        print_errno("could not unmap index file");
    }
    close_file(fd);
}

// character map. A -> T, C -> G, G -> C, T -> A.
static const char canonicalize_basepair_map[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 84, 0, 71, 0, 0, 0, 67, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 65, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const char * canonicalize_kmer(const char* query_8, char* kmer_buffer, uint32_t kmer_size) {
    const char* map = canonicalize_basepair_map;
    const uint8_t* query_8_reverse =
        reinterpret_cast<const uint8_t*>(query_8) + kmer_size - 1;
    size_t i = 0;
    while (query_8[i] == map[*(query_8_reverse - i)] && i < kmer_size / 2) {
        i++;
    }

    if (query_8[i] <= map[*(query_8_reverse - i)]) {
        return query_8;
    }
    else {
        for (size_t j = 0; j < kmer_size; j++) {
            kmer_buffer[kmer_size - j - 1] =
                map[static_cast<uint8_t>(query_8[j])];
        }
        kmer_buffer[kmer_size] = 0;
        return kmer_buffer;
    }
}

} // namespace cobs::query

/******************************************************************************/
