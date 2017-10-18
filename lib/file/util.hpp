#pragma once

#include "sample.hpp"
#include "bloom_filter.hpp"
#include "bloom_filter_header.hpp"
#include "sample_header.hpp"
#include "msbf_header.hpp"

namespace genome::file {
    static sample_header dummy_sh;
    static bloom_filter_header dummy_bfh;
    static msbf_header dummy_msbfh;

    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s);
    template<uint32_t N>
    void serialize(const boost::filesystem::path& p, const sample<N>& s);
    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s, sample_header& sh = dummy_sh);
    template<uint32_t N>
    void deserialize(const boost::filesystem::path& p, sample<N>& s, sample_header& sh = dummy_sh);

    void serialize(std::ofstream& ofs, const bloom_filter& bf, const std::vector<std::string>& file_names);
    void serialize(const boost::filesystem::path& p, const bloom_filter& bf, const std::vector<std::string>& file_names);
    void deserialize(std::ifstream& ifs, bloom_filter& bf, bloom_filter_header& bfh = dummy_bfh);
    void deserialize(const boost::filesystem::path& p, bloom_filter& bf, bloom_filter_header& bfh = dummy_bfh);

    void deserialize(std::ifstream& ifs, std::vector<std::vector<byte>>& data, msbf_header& msbfh = dummy_msbfh);
    void deserialize(const boost::filesystem::path& p, std::vector<std::vector<byte>>& data, msbf_header& msbfh = dummy_msbfh);


    template<class T>
    void serialize_header(std::ofstream& ofs, const boost::filesystem::path& p, const T& h);
    template<class T>
    T deserialize_header(std::ifstream& ifs, const boost::filesystem::path& p);
}

#include "util.tpp"
