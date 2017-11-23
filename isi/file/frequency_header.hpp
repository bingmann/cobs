#pragma once

#include <isi/file/header.hpp>

namespace isi::file {
    class frequency_header : public header<frequency_header> {
    protected:
        void serialize(std::ofstream& ost) const override;
        void deserialize(std::ifstream& ifs) override;
    public:
        static const std::string magic_word;
        static const std::string file_extension;
    };
}



