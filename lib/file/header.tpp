#include "header.hpp"
#include "classic_index_header.hpp"
#include <util.hpp>

namespace isi::file {
    template<class T>
    const std::string header<T>::magic_word = "HEADER";
    template<class T>
    const uint32_t header<T>::m_version = 1;

    template<class T>
    void header<T>::serialize(std::ofstream& ofs, const header& h) {
        ofs << magic_word;
        isi::serialize(ofs, m_version);
        h.serialize(ofs);
        ofs << T::magic_word;
    }

    template<class T>
    void header<T>::deserialize(std::ifstream& ifs, header& h) {
        check_magic_word(ifs, magic_word);
        uint32_t v;
        isi::deserialize(ifs, v);
        assert(v == m_version);
        h.deserialize(ifs);
        check_magic_word(ifs, T::magic_word);
    }

    template<class T>
    void header<T>::check_magic_word(std::ifstream& ifs, const std::string& magic_word) {
        std::vector<char> mw_v(magic_word.size(), ' ');
        ifs.read(mw_v.data(), magic_word.size());
        std::string mw(mw_v.data(), mw_v.size());
        assert(ifs.good());
        assert(mw == magic_word);
    }
}
