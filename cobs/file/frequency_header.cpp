#include <cobs/file/frequency_header.hpp>

namespace cobs::file {
    const std::string frequency_header::magic_word = "FREQUENCY";
    const std::string frequency_header::file_extension = ".freq.isi";
    void frequency_header::serialize(std::ofstream& /*ost*/) const {}
    void frequency_header::deserialize(std::ifstream& /*ifs*/) {}
}
