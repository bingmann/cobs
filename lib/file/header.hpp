#pragma once

#include <string>
#include <ostream>

namespace genome::file {
    template<class T>
    class header {
    private:
        static const uint32_t m_version;
        static void check_magic_word(std::ifstream& ifs, const std::string& magic_word);
    protected:
        virtual void serialize(std::ofstream& ofs) const = 0;
        virtual void deserialize(std::ifstream& ifs) = 0;
    public:
        static const std::string magic_word;
        static void serialize(std::ofstream& ofs, const header& h);
        static void deserialize(std::ifstream& ifs, header& h);
    };
}

#include "header.tpp"
