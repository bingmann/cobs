#pragma once

#include <cassert>
#include <fstream>
#include <experimental/filesystem>

namespace isi {

    struct stream_metadata {
        uint64_t curr_pos;
        uint64_t end_pos;
    };

    stream_metadata get_stream_metadata(std::ifstream& ifs);

    template<typename T>
    inline void read_file(const std::experimental::filesystem::path& path, std::vector<T>& v) {
        std::ifstream ifs(path.string(), std::ios::in | std::ios::binary);
        stream_metadata smd = get_stream_metadata(ifs);
        assert(smd.end_pos % sizeof(T) == 0);
        v.resize(smd.end_pos / sizeof(T));
        ifs.read(reinterpret_cast<char*>(v.data()), smd.end_pos);
    }

    template<typename T>
    inline void write(std::ostream& ost, const T& t) {
        ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    void serialize(std::ostream& /*ost*/);

    template<typename T, typename... Args>
    inline void serialize(std::ostream& ost, const T& t, const Args&... args) {
        ost.write(reinterpret_cast<const char*>(&t), sizeof(T));
        serialize(ost, args...);
    }

    void deserialize(std::istream& /*ifs*/);

    template<typename T, typename... Args>
    inline void deserialize(std::istream& ifs, T& t, Args&... args) {
        ifs.read(reinterpret_cast<char*>(&t), sizeof(T));
        deserialize(ifs, args...);
    }
}
