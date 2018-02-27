#pragma once

#include <experimental/filesystem>
#include <algorithm>
#include <iostream>

#include <cobs/sample.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/file/sample_header.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/file/frequency_header.hpp>

namespace cobs::file {
    static sample_header dummy_sh;
    static classic_index_header dummy_isih;
    static compact_index_header dummy_cisih;

    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s, const std::string& name);

    template<uint32_t N>
    void serialize(const std::experimental::filesystem::path& p, const sample<N>& s, const std::string& name);

    void serialize(std::ofstream& ofs, const std::vector<uint8_t>& data, const classic_index_header& h);
    void serialize(const std::experimental::filesystem::path& p, const std::vector<uint8_t>& data, const classic_index_header& h);

    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s, sample_header& h = dummy_sh);

    template<uint32_t N>
    void deserialize(const std::experimental::filesystem::path& p, sample<N>& s, sample_header& h = dummy_sh);

    void deserialize(std::ifstream& ifs, std::vector<uint8_t>& data, classic_index_header& h = dummy_isih);
    void deserialize(std::ifstream& ifs, std::vector<std::vector<uint8_t>>& data, compact_index_header& h = dummy_cisih);
    void deserialize(const std::experimental::filesystem::path& p, std::vector<uint8_t>& data, classic_index_header& h = dummy_isih);
    void deserialize(const std::experimental::filesystem::path& p, std::vector<std::vector<uint8_t>>& data, compact_index_header& h = dummy_cisih);

    template<class T>
    void serialize_header(std::ofstream& ofs, const std::experimental::filesystem::path& p, const T& h);

    template<class T>
    void serialize_header(const std::experimental::filesystem::path& p, const T& h);

    template<class T>
    T deserialize_header(std::ifstream& ifs, const std::experimental::filesystem::path& p);

    template<class T>
    T deserialize_header(const std::experimental::filesystem::path& p);

    template<class T>
    bool file_is(const std::experimental::filesystem::path& p);

    std::string file_name(const std::experimental::filesystem::path& p);
}


namespace cobs::file {
    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s, const std::string& name) {
        ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        sample_header sh(name, N);
        header<sample_header>::serialize(ofs, sh);
        ofs.write(reinterpret_cast<const char*>(s.data().data()), kmer<N>::size * s.data().size());
    }

    template<uint32_t N>
    void serialize(const std::experimental::filesystem::path& p, const sample<N>& s, const std::string& name) {
        std::experimental::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, s, name);
    }


    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s, sample_header& h) {
        ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        header<sample_header>::deserialize(ifs, h);
        assert(N == h.kmer_size());

        stream_metadata smd = get_stream_metadata(ifs);
        size_t size = smd.end_pos - smd.curr_pos;
        s.data().resize(size / kmer<N>::size);
        ifs.read(reinterpret_cast<char*>(s.data().data()), size);
    }

    template<uint32_t N>
    void deserialize(const std::experimental::filesystem::path& p, sample<N>& s, sample_header& h) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, s, h);
    }

    template<class T>
    void serialize_header(std::ofstream& ofs, const std::experimental::filesystem::path& p, const T& h) {
        ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        ofs.open(p.string(), std::ios::out | std::ios::binary);
        header<T>::serialize(ofs, h);
    }

    template<class T>
    void serialize_header(const std::experimental::filesystem::path& p, const T& h) {
        std::ofstream ofs;
        serialize_header<T>(ofs, p, h);
    }

    template<class T>
    T deserialize_header(std::ifstream& ifs, const std::experimental::filesystem::path& p) {
        ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        ifs.open(p.string(), std::ios::in | std::ios::binary);
        T h;
        header<T>::deserialize(ifs, h);
        return h;
    }

    template<class T>
    T deserialize_header(const std::experimental::filesystem::path& p) {
        std::ifstream ifs;
        return deserialize_header<T>(ifs, p);
    }

    template<class T>
    bool file_is(const std::experimental::filesystem::path& p) {
        bool result = std::experimental::filesystem::is_regular_file(p);
        if (result) {
            try {
                deserialize_header<T>(p);
            } catch (...) {
                std::cout << p.string() << " is not a " << typeid(T).name() << std::endl;
                result = false;
            }
        }

        return result;
    }
}

