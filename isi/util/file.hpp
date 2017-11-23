#pragma once

#include <experimental/filesystem>
#include <algorithm>
#include <iostream>

#include <isi/sample.hpp>
#include <isi/file/classic_index_header.hpp>
#include <isi/file/sample_header.hpp>
#include <isi/file/compact_index_header.hpp>
#include <isi/file/frequency_header.hpp>

namespace isi::file {
    static sample_header dummy_sh;
    static classic_index_header dummy_isih;
    static compact_index_header dummy_cisih;

    template<uint32_t N>
    void serialize(std::ofstream& ofs, const sample<N>& s) {
        sample_header sh(N);
        header<sample_header>::serialize(ofs, sh);
        ofs.write(reinterpret_cast<const char*>(s.data().data()), kmer<N>::size * s.data().size());
    }

    template<uint32_t N>
    void serialize(const std::experimental::filesystem::path& p, const sample<N>& s) {
        std::experimental::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, s);
    }

    inline void serialize(std::ofstream& ofs, const std::vector<uint8_t>& data, const classic_index_header& h) {
        header<classic_index_header>::serialize(ofs, h);
        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    inline void serialize(const std::experimental::filesystem::path& p, const std::vector<uint8_t>& data, const classic_index_header& h) {
        std::experimental::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, data, h);
    }

    template<uint32_t N>
    void deserialize(std::ifstream& ifs, sample<N>& s, sample_header& h = dummy_sh) {
        header<sample_header>::deserialize(ifs, h);
        assert(N == h.kmer_size());

        stream_metadata smd = get_stream_metadata(ifs);
        size_t size = smd.end_pos - smd.curr_pos;
        s.data().resize(size / kmer<N>::size);
        ifs.read(reinterpret_cast<char*>(s.data().data()), size);
    }

    template<uint32_t N>
    void deserialize(const std::experimental::filesystem::path& p, sample<N>& s, sample_header& h = dummy_sh) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, s, h);
    }

    inline void deserialize(std::ifstream& ifs, std::vector<uint8_t>& data, classic_index_header& h = dummy_isih) {
        header<classic_index_header>::deserialize(ifs, h);
        stream_metadata smd = get_stream_metadata(ifs);
        size_t size = smd.end_pos - smd.curr_pos;
        data.resize(size);
        ifs.read(reinterpret_cast<char*>(data.data()), size);
    }

    inline void deserialize(std::ifstream& ifs, std::vector<std::vector<uint8_t>>& data, compact_index_header& h = dummy_cisih) {
        header<compact_index_header>::deserialize(ifs, h);
        data.clear();
        data.resize(h.parameters().size());
        for (size_t i = 0; i < h.parameters().size(); i++) {
            size_t data_size = h.page_size() * h.parameters()[i].signature_size;
            std::vector<uint8_t> d(data_size);
            ifs.read(reinterpret_cast<char*>(d.data()), data_size);
            data[i] = std::move(d);
        }
    }

    inline void deserialize(const std::experimental::filesystem::path& p, std::vector<uint8_t>& data, classic_index_header& h = dummy_isih) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, data, h);
    }

    inline void deserialize(const std::experimental::filesystem::path& p, std::vector<std::vector<uint8_t>>& data, compact_index_header& h = dummy_cisih) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, data, h);
    }

    template<class T>
    void serialize_header(std::ofstream& ofs, const std::experimental::filesystem::path& p, const T& h) {
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
        try {
            deserialize_header<T>(p);
            return true;
        } catch (file_io_exception& /*e*/) {
            return false;
        }
    }

    inline std::string file_name(const std::experimental::filesystem::path& p) {
        std::string result = p.filename();
        auto comp = [](char c){
            return c == '.';
        };
        auto iter = std::find_if(result.rbegin(), result.rend(), comp) + 1;
        iter = std::find_if(iter, result.rend(), comp) + 1;
        return result.substr(0, std::distance(iter, result.rend()));
    }
}

