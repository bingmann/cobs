/*******************************************************************************
 * cobs/util/parameters.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cmath>
#include <cobs/util/parameters.hpp>

#include <tlx/die.hpp>

namespace cobs {

double calc_signature_size_ratio(double num_hashes, double false_positive_probability) {
    double denominator = std::log(1 - std::pow(false_positive_probability, 1 / num_hashes));
    double result = -num_hashes / denominator;
    die_unless(result > 0);
    return result;
}

uint64_t calc_signature_size(uint64_t num_elements, double num_hashes, double false_positive_probability) {
    double signature_size_ratio = calc_signature_size_ratio(num_hashes, false_positive_probability);
    double result = std::ceil(num_elements * signature_size_ratio);
    die_unless(result >= 0);
    die_unless(result <= UINT64_MAX);
    return (uint64_t)result;
}

double calc_average_set_bit_ratio(uint64_t signature_size, double num_hashes, double false_positive_probability) {
    double num_elements = signature_size / calc_signature_size_ratio(num_hashes, false_positive_probability);
    double result = 1 - std::pow(1 - 1 / (double)signature_size, num_hashes * num_elements);
    die_unless(result >= 0);
    die_unless(result <= 1);
    return result;
}

} // namespace cobs

/******************************************************************************/
