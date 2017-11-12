#pragma once

#include "sample.hpp"
#include "isi/classic_index.hpp"
#include "classic_index_header.hpp"
#include "sample_header.hpp"
#include "compact_index_header.hpp"

namespace isi::file {
    static sample_header dummy_sh;
    static classic_index_header dummy_clah;
    static compact_index_header dummy_comh;

    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s);
    template<uint32_t N>
    void serialize(const std::experimental::filesystem::path& p, const sample<N>& s);
    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s, sample_header& sh = dummy_sh);
    template<uint32_t N>
    void deserialize(const std::experimental::filesystem::path& p, sample<N>& s, sample_header& sh = dummy_sh);

    void serialize(std::ofstream& ofs, const classic_index& bf, const std::vector<std::string>& file_names);
    void serialize(const std::experimental::filesystem::path& p, const classic_index& bf, const std::vector<std::string>& file_names);
    void deserialize(std::ifstream& ifs, classic_index& bf, classic_index_header& h = dummy_clah);
    void deserialize(const std::experimental::filesystem::path& p, classic_index& bf, classic_index_header& h = dummy_clah);

    void deserialize(std::ifstream& ifs, std::vector<std::vector<uint8_t>>& data, compact_index_header& h = dummy_comh);
    void deserialize(const std::experimental::filesystem::path& p, std::vector<std::vector<uint8_t>>& data, compact_index_header& h = dummy_comh);


    template<class T>
    void serialize_header(std::ofstream& ofs, const std::experimental::filesystem::path& p, const T& h);
    template<class T>
    T deserialize_header(std::ifstream& ifs, const std::experimental::filesystem::path& p);
}

#include "util.tpp"
