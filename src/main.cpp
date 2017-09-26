#include <file_format/bloom_filter_header.hpp>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    genome::file_format::bloom_filter_header a(200000, 1, 7);
    std::cout << sizeof(genome::file_format::bloom_filter_header);
    std::ofstream ofs("test.tt", std::ios::out | std::ios::binary);
    ofs << a;
}
