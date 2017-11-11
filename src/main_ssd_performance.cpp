#include <fcntl.h>
#include <sys/mman.h>
#include <fstream>
#include <timer.hpp>
#include <iostream>
#include <omp.h>
#include <stxxl/aligned_alloc>
#include <stxxl/bits/io/syscall_file.h>
#include <stxxl/request>
#include <experimental/filesystem>
#include <cstring>

void clear_caches() {
    sync();
    std::ofstream ofs("/proc/sys/vm/drop_caches");
    ofs << "3" << std::endl;
}

char test(stxxl::syscall_file& file, uint64_t page_size, size_t num_access, size_t file_size, isi::timer& t) {
    auto* data = reinterpret_cast<char*>(stxxl::aligned_alloc<4096>(num_access * page_size));
//    std::vector<char> data(num_access * page_size);
    std::srand(std::time(nullptr));
    std::vector<size_t> offsets(num_access);
    std::vector<char*> dst(num_access);

    for (size_t i = 0; i < num_access; i++) {
        offsets[i] = page_size * (std::rand() % (file_size / page_size));
        dst[i] = data + i * page_size;
    }

    std::vector<stxxl::request_ptr> requests(num_access);
    t.active("asio " + std::to_string(page_size) + " " + std::to_string(num_access));
    for(size_t i = 0; i < num_access; i++) {
        requests[i] = file.aread(dst[i], offsets[i], page_size);
    }
    stxxl::wait_all(requests.data(), requests.size());
    t.stop();

    char sum;
    for(size_t i = 0; i < num_access * page_size; i++) {
        sum += data[i];
    }
    stxxl::aligned_dealloc<4096>(data);
    return sum;
}

char test(char* file, uint64_t page_size, size_t num_access, size_t file_size, isi::timer& t) {
    auto* data = reinterpret_cast<char*>(stxxl::aligned_alloc<4096>(num_access * page_size));
//    std::vector<char> data(num_access * page_size);
    std::srand(std::time(nullptr));
    std::vector<char*> src(num_access);
    std::vector<char*> dst(num_access);

    for (size_t i = 0; i < num_access; i++) {
        src[i] = file + page_size * (std::rand() % (file_size / page_size));
        dst[i] = data + i * page_size;
    }

    t.active("mmap " + std::to_string(page_size) + " " + std::to_string(num_access));
//    omp_set_dynamic(0);
    #pragma omp parallel for num_threads(32)
    for(size_t i = 0; i < num_access; i++) {
        std::memcpy(dst[i], src[i], page_size);
    }
    t.stop();

    char sum;
    for(size_t i = 0; i < num_access * page_size; i++) {
        sum += data[i];
    }
    stxxl::aligned_dealloc<4096>(data);
    return sum;
}

void test(char* file_mmap, stxxl::syscall_file& file_asio, size_t file_size) {
    isi::timer t;
    char sum;
    size_t max = std::pow(2, 14);
    for (size_t i = 1; i <= max; i*=2) {
        clear_caches();
        sum += test(file_mmap, i * getpagesize(), max / i, file_size, t);
        clear_caches();
//        sum += test(file_asio, i * getpagesize(), max / i, file_size, t);
    }
    std::cout << t << std::endl << sum;
}

int main() {
    std::experimental::filesystem::path p("/users/flo/projects/thesis/data/performance_bloom/large.g_isi");

    uint64_t end_pos;
    {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        std::streamoff curr_pos = ifs.tellg();
        ifs.seekg(0, std::ios::end);
        end_pos = (uint64_t) ifs.tellg();
    }

    int fd = open(p.string().data(), O_RDONLY, 0);
    assert(fd != -1);

    char* file_mmap = reinterpret_cast<char*>(mmap(NULL, end_pos, PROT_READ, MAP_PRIVATE, fd, 0));

    assert(madvise(file_mmap, end_pos, MADV_RANDOM) == 0);
    stxxl::syscall_file file_asio(p.string(), stxxl::file::RDONLY);
    test(file_mmap, file_asio, end_pos);
};
