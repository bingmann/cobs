#include "frequency_header.hpp"

namespace genome::file {
    const std::string frequency_header::magic_word = "FREQUENCY";
    const std::string frequency_header::file_extension = ".g_fre";
    void frequency_header::serialize(std::ofstream& ost) const {}
    void frequency_header::deserialize(std::ifstream& ifs) {}
}
