#pragma once

#include "sample.hpp"
#include "bloom_filter.hpp"

namespace genome::file {
    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s);
    template<uint32_t N>
    void serialize(const boost::filesystem::path& p, const sample<N>& s);
    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s);
    template<uint32_t N>
    void deserialize(const boost::filesystem::path& p, sample<N>& s);

    void serialize(std::ofstream& ofs, const bloom_filter& bf);
    void serialize(const boost::filesystem::path& p, const bloom_filter& bf);
    void deserialize(std::ifstream& ifs, bloom_filter& bf);
    void deserialize(const boost::filesystem::path& p, bloom_filter& bf);


    template<class T>
    void serialize_header(std::ofstream& ofs, const boost::filesystem::path& p, const T& h);
    template<class T>
    T deserialize_header(std::ifstream& ifs, const boost::filesystem::path& p);
}

#include "util.tpp"
