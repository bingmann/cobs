#include <isi/util/query.hpp>
#include <isi/util/error_handling.hpp>
#include <zconf.h>
#include <algorithm>
#include <iostream>

namespace isi::query {
    std::pair<int, uint8_t*> initialize_mmap(const std::experimental::filesystem::path& path, const stream_metadata& smd) {
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

    void destroy_mmap(int fd, uint8_t* mmap_ptr, const stream_metadata& smd) {
        if (munmap(mmap_ptr - smd.curr_pos, smd.end_pos)) {
            print_errno("could not unmap index file");
        }
        if (close(fd)) {
            print_errno("could not close index file");
        }
    }

    std::unordered_map<char, char> basepairs = {{'A', 'T'}, {'C', 'G'}, {'G', 'C'}, {'T', 'A'}};
    const char* canonicalize_kmer(const char* query_8, char* kmer_raw_8, uint32_t kmer_size) {
        const char* query_8_reverse = query_8 + kmer_size - 1;
        size_t i = 0;
        while(query_8[i] == basepairs[*(query_8_reverse - i)] && i < kmer_size / 2) {
            i++;
        }

        if(query_8[i] <= basepairs[*(query_8_reverse - i)]) {
            return query_8;
        } else {
            for (size_t j = 0; j < kmer_size; j++) {
                kmer_raw_8[kmer_size - j - 1] = basepairs.at(query_8[j]);
            }
            return kmer_raw_8;
        }
    }
}
