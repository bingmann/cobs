#pragma once

#include <cstdint>

namespace cobs {
    double calc_signature_size_ratio(double num_hashes, double false_positive_probability);
    uint64_t calc_signature_size(uint64_t num_elements, double num_hashes, double false_positive_probability);
    double calc_average_set_bit_ratio(uint64_t signature_size, double num_hashes, double false_positive_probability);
}
