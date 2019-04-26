/*******************************************************************************
 * cobs/util/query.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/settings.hpp>
#include <cobs/util/error_handling.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/query.hpp>

#include <algorithm>
#include <iostream>
#include <unistd.h>

#include <tlx/logger.hpp>
#include <tlx/string/format_iec_units.hpp>

namespace cobs {

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

MMapHandle initialize_mmap(const fs::path& path)
{
    int fd = open_file(path, O_RDONLY);
    off_t size = lseek(fd, 0, SEEK_END);

    if (!gopt_load_complete_index) {
        void* mmap_ptr = mmap(nullptr, size, PROT_READ,
                              MAP_PRIVATE, fd, /* offset */ 0);
        if (mmap_ptr == MAP_FAILED) {
            exit_error_errno("mmap failed");
        }
        if (madvise(mmap_ptr, size, MADV_RANDOM)) {
            print_errno("madvise failed for MADV_RANDOM");
        }
        return MMapHandle {
                   fd, reinterpret_cast<uint8_t*>(mmap_ptr), uint64_t(size)
        };
    }
    else {
        LOG1 << "Reading complete index";
        uint8_t* data_ptr = reinterpret_cast<uint8_t*>(
            std::aligned_alloc(2 * 1024 * 1024, size));
        if (madvise(data_ptr, size, MADV_HUGEPAGE)) {
            print_errno("madvise failed for MADV_HUGEPAGE");
        }
        lseek(fd, 0, SEEK_SET);
        uint64_t remain = size;
        size_t pos = 0;
        while (remain != 0) {
            ssize_t rb = read(fd, data_ptr + pos, remain);
            if (rb < 0) {
                print_errno("read failed");
                break;
            }
            remain -= rb;
            pos += rb;
            LOG1 << "Read " << tlx::format_iec_units(pos)
                 << "B / " << tlx::format_iec_units(size) << "B - "
                 << pos * 100 / size << "%";
        }
        LOG1 << "Index loaded into RAM.";
        return MMapHandle {
                   fd, data_ptr, uint64_t(size)
        };
    }
}

void destroy_mmap(MMapHandle& handle)
{
    if (!gopt_load_complete_index) {
        if (munmap(handle.data, handle.size)) {
            print_errno("could not unmap index file");
        }
    }
    else {
        free(handle.data);
    }
    close_file(handle.fd);
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

} // namespace cobs

/******************************************************************************/
