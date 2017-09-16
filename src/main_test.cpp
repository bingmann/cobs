#include <array>
#include <iostream>
#include <vector>
#include <helpers.hpp>
#include <stdexcept>


template<typename size_type, typename ForwardIterator>
size_type cast(ForwardIterator iter, size_t offset = 0) {
    return *reinterpret_cast<size_type*>(std::next(iter, offset));
}

struct header {
    uint32_t num_words_per_kmer;
    uint32_t num_colors;
};

template<typename ForwardIterator>
void check_magic_number(ForwardIterator& iter) {
    std::string magic_word = "CORTEX";
    if (magic_word != std::string(iter, std::next(iter, magic_word.size()))) {
        throw std::invalid_argument("magic number does not match");
    }
    std::advance(iter, 6);
}

template<typename ForwardIterator>
header skip_header(ForwardIterator iter) {
    check_magic_number(iter);
    std::advance(iter, 8);
    auto num_words_per_kmer  = cast<uint32_t>(iter);
    std::advance(iter, 4);
    auto num_colors = cast<uint32_t>(iter);
    std::advance(iter, 16 * num_colors);
    for (size_t i = 0; i < num_colors; i++) {
        auto sample_name_length = cast<uint32_t>(iter);
        std::advance(iter, 4 + sample_name_length);
    }
    std::advance(iter, 16 * num_colors);
    for (size_t i = 0; i < num_colors; i++) {
        std::advance(iter, 12);
        auto length_graph_name = cast<uint32_t>(iter);
        std::advance(iter, 4 + length_graph_name);
    }
    check_magic_number(iter);
    return {num_words_per_kmer, num_colors};
}

int main(int argc, char** argv) {
    std::string path = "test/resources/sample.ctx";
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    ifs.seekg(0, std::ios_base::end);
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);
    std::vector<char> v(size);
    ifs.read(v.data(), size);
    ifs.close();
    auto header = skip_header(v.data());

    std::cout << std::string(v.data(), 6) << std::endl;

    auto version = cast<uint32_t>(v.data(), 6);
    size_t offset = 10;
    auto kmer_size = cast<uint32_t>(v.data(), offset);
    offset += 4;
    auto num_words_per_kmer = cast<uint32_t>(v.data(), offset);
    offset += 4;
    auto num_colors = cast<uint32_t>(v.data(), offset);
    offset += 4;

    auto read_length = cast<uint32_t>(v.data(), offset);
    auto total_seq = cast<uint64_t >(v.data(), offset + 4);
    offset += 12 * num_colors;

    for (size_t i = 0; i < num_colors; i++) {
        auto sample_name_length = cast<uint32_t>(v.data(), offset);
        std::cout << std::string(v.data() + offset + 4, sample_name_length) << std::endl;
        offset += 4 + sample_name_length;
    }
    auto s = cast<long double>(v.data(), offset);
    size_t p = sizeof(long double);
    offset += 16 * num_colors;

    auto b = cast<uint8_t>(v.data(), offset);
    auto c = cast<uint8_t>(v.data(), offset + 1);
    auto d = cast<uint8_t>(v.data(), offset + 2);
    auto e = cast<uint8_t>(v.data(), offset + 3);
    auto f = cast<uint32_t>(v.data(), offset + 4);
    auto g = cast<uint32_t>(v.data(), offset + 8);
    auto h = cast<uint32_t>(v.data(), offset + 12);
    for (size_t i = 0; i < num_colors; i++) {
        offset += 12;
        auto length_graph_name = cast<uint32_t>(v.data(), offset);
        std::cout << std::string(v.data() + 4 + offset, length_graph_name) << std::endl;
        offset += 4 + length_graph_name;
    }

    std::cout << std::string(v.data() + offset, 6) << std::endl;

    auto i = cast<uint32_t>(v.data(), offset + 2);
    std::cout << std::string(v.data(), offset + 12) << std::endl;
    std::cout << std::string(v.data() + 38, 7) << std::endl;
    auto j = cast<uint32_t>(v.data(), offset + 34);
    int a = 0;
}
