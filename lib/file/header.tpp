#include "header.hpp"
#include "bloom_filter_header.hpp"
#include <helpers.hpp>

namespace genome::file {
    template<class T>
    const std::string header<T>::m_magic_word = "HEADER";
    template<class T>
    const uint32_t header<T>::m_version = 1;

    template<class T>
    void header<T>::serialize(std::ofstream& ofs, const header& h) {
        ofs << m_magic_word;
        genome::serialize(ofs, m_version);
        h.serialize(ofs);
        ofs << T::magic_word;
    }

    template<class T>
    void header<T>::deserialize(std::ifstream& ifs, header& h) {
        check_magic_word(ifs, m_magic_word);
        uint32_t v;
        genome::deserialize(ifs, v);
        assert(v == m_version);
        h.deserialize(ifs);
        check_magic_word(ifs, T::magic_word);
    }

    template<class T>
    void header<T>::check_magic_word(std::ifstream& ifs, const std::string& magic_word) {
        std::string mw(magic_word.size(), ' ');
        ifs.read(mw.data(), magic_word.size());
        assert(mw == magic_word);
    }
}
