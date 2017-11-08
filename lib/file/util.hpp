#pragma once

#include "sample.hpp"
#include "bit_sliced_signatures/bss.hpp"
#include "bss_header.hpp"
#include "sample_header.hpp"
#include "abss_header.hpp"

namespace genome::file {
    static sample_header dummy_sh;
    static bss_header dummy_bssh;
    static abss_header dummy_abssh;

    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s);
    template<uint32_t N>
    void serialize(const std::experimental::filesystem::path& p, const sample<N>& s);
    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s, sample_header& sh = dummy_sh);
    template<uint32_t N>
    void deserialize(const std::experimental::filesystem::path& p, sample<N>& s, sample_header& sh = dummy_sh);

    void serialize(std::ofstream& ofs, const bss& bf, const std::vector<std::string>& file_names);
    void serialize(const std::experimental::filesystem::path& p, const bss& bf, const std::vector<std::string>& file_names);
    void deserialize(std::ifstream& ifs, bss& bf, bss_header& bssh = dummy_bssh);
    void deserialize(const std::experimental::filesystem::path& p, bss& bf, bss_header& bssh = dummy_bssh);

    void deserialize(std::ifstream& ifs, std::vector<std::vector<byte>>& data, abss_header& abssh = dummy_abssh);
    void deserialize(const std::experimental::filesystem::path& p, std::vector<std::vector<byte>>& data, abss_header& abssh = dummy_abssh);


    template<class T>
    void serialize_header(std::ofstream& ofs, const std::experimental::filesystem::path& p, const T& h);
    template<class T>
    T deserialize_header(std::ifstream& ifs, const std::experimental::filesystem::path& p);
}

#include "util.tpp"
