#include <isi/util/file.hpp>

namespace isi::file {
    void serialize(std::ofstream& ofs, const std::vector<uint8_t>& data, const classic_index_header& h) {
        ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        header<classic_index_header>::serialize(ofs, h);
        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    void serialize(const std::experimental::filesystem::path& p, const std::vector<uint8_t>& data, const classic_index_header& h) {
        std::experimental::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p.string(), std::ios::out | std::ios::binary);
        serialize(ofs, data, h);
    }

    void deserialize(std::ifstream& ifs, std::vector<uint8_t>& data, classic_index_header& h) {
        ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        header<classic_index_header>::deserialize(ifs, h);
        stream_metadata smd = get_stream_metadata(ifs);
        size_t size = smd.end_pos - smd.curr_pos;
        data.resize(size);
        ifs.read(reinterpret_cast<char*>(data.data()), size);
    }

    void deserialize(std::ifstream& ifs, std::vector<std::vector<uint8_t>>& data, compact_index_header& h) {
        ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
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

    void deserialize(const std::experimental::filesystem::path& p, std::vector<uint8_t>& data, classic_index_header& h) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, data, h);
    }

    void deserialize(const std::experimental::filesystem::path& p, std::vector<std::vector<uint8_t>>& data, compact_index_header& h) {
        std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
        deserialize(ifs, data, h);
    }

    std::string file_name(const std::experimental::filesystem::path& p) {
        std::string result = p.filename();
        auto comp = [](char c){
            return c == '.';
        };
        auto iter = std::find_if(result.rbegin(), result.rend(), comp) + 1;
        iter = std::find_if(iter, result.rend(), comp) + 1;
        return result.substr(0, std::distance(iter, result.rend()));
    }
}
