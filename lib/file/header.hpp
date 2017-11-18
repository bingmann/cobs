#pragma once

#include <string>
#include <ostream>
#include <util.hpp>
#include <vector>

namespace isi::file {
    template<class T>
    class header {
    private:
        static const uint32_t m_version;

        static void check_magic_word(std::ifstream& ifs, const std::string& magic_word) {
            std::vector<char> mw_v(magic_word.size(), ' ');
            ifs.read(mw_v.data(), magic_word.size());
            std::string mw(mw_v.data(), mw_v.size());
            assert(ifs.good());
            assert_exit(mw == magic_word, "invalid file type");
        }

    protected:
        virtual void serialize(std::ofstream& ofs) const = 0;
        virtual void deserialize(std::ifstream& ifs) = 0;
    public:
        static const std::string magic_word;

        static void serialize(std::ofstream& ofs, const header& h) {
            ofs << magic_word;
            isi::serialize(ofs, m_version);
            h.serialize(ofs);
            ofs << T::magic_word;
        }

        static void deserialize(std::ifstream& ifs, header& h) {
            check_magic_word(ifs, magic_word);
            uint32_t v;
            isi::deserialize(ifs, v);
            assert_exit(v == m_version, "invalid file version");
            h.deserialize(ifs);
            check_magic_word(ifs, T::magic_word);
        }

    };

    template<class T>
    const std::string header<T>::magic_word = "INSIIN";
    template<class T>
    const uint32_t header<T>::m_version = 1;
}
