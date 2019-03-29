/*******************************************************************************
 * cobs/util/calc_signature_size.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_CALC_SIGNATURE_SIZE_HEADER
#define COBS_UTIL_CALC_SIGNATURE_SIZE_HEADER

#include <cstdint>

namespace cobs {

//! calculate the size ratio of a Bloom filter with k hash functions and given
//! fpr.
double calc_signature_size_ratio(double num_hashes,
                                 double false_positive_probability);

//! calculate the number of cells in a Bloom filter with k hash functions into
//! which num_elements are inserted such that it has expected given fpr.
uint64_t calc_signature_size(uint64_t num_elements, double num_hashes,
                             double false_positive_probability);

//! calculate expected probability of a bit in the Bloom filter to be one
double calc_average_set_bit_ratio(uint64_t signature_size, double num_hashes,
                                  double false_positive_probability);

} // namespace cobs

#endif // !COBS_UTIL_CALC_SIGNATURE_SIZE_HEADER

/******************************************************************************/
