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
#include <cstdlib>
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
        void* ptr = nullptr;
        if (posix_memalign(&ptr, 2 * 1024 * 1024, size)) {
            print_errno("posix_memalign()");
        }
        uint8_t* data_ptr = reinterpret_cast<uint8_t*>(ptr);
#if defined(MADV_HUGEPAGE)
        if (madvise(data_ptr, size, MADV_HUGEPAGE)) {
            print_errno("madvise failed for MADV_HUGEPAGE");
        }
#endif
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

//! forward character map. A -> A, C -> C, G -> G, T -> T. rest maps to zero.
static const char canonicalize_basepair_forward_map[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 65, 0, 67, 0, 0, 0, 71, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 84, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

//! reverse character map. A -> T, C -> G, G -> C, T -> A. rest maps to zero.
static const char canonicalize_basepair_reverse_map[256] = {
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

bool canonicalize_kmer(const char* input, char* output, size_t size)
{
    const char* fmap = canonicalize_basepair_forward_map;
    const char* rmap = canonicalize_basepair_reverse_map;

    bool good = true;

    const uint8_t* finput = reinterpret_cast<const uint8_t*>(input);
    const uint8_t* rinput = finput + size - 1;
    char* foutput = output;

    size_t i = 0;
    for ( ; i < size / 2; ++i) {
        // map values at forward and reverse pointers
        char f = fmap[*(finput + i)];
        char r = rmap[*(rinput - i)];

        *foutput++ = f;

        // good canonicalization if both are not mapped to zero.
        good = good && (f != 0) && (r != 0);

        if (f < r) {
            // the input is the lexicographic smaller k-mer. keep it.

            // translate remaining input
            for (++i; i < size; ++i) {
                char f = fmap[*(finput + i)];
                *foutput++ = f;
                good = good && (f != 0);
            }
            return good;
        }
        else if (f > r) {
            // calculate reverse k-mer and check input while reversing
            for (size_t j = 0; j < size; j++) {
                char x = rmap[finput[j]];
                output[size - j - 1] = x;
                good = good && (x != 0);
            }
            return good;
        }

        // else f == r and we continue the scan
    }

    // the input is the lexicographic smaller k-mer. keep it. special case for
    // odd number k-mer sizes

    // translate remaining input
    for ( ; i < size; ++i) {
        char f = fmap[*(finput + i)];
        *foutput++ = f;
        good = good && (f != 0);
    }
    return good;
}

} // namespace cobs

/******************************************************************************/
