/*******************************************************************************
 * cobs/file/frequency_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/frequency_header.hpp>

namespace cobs {

const std::string FrequencyHeader::magic_word = "FREQUENCY";
const std::string FrequencyHeader::file_extension = ".freq.isi";
void FrequencyHeader::serialize(std::ofstream& /*ost*/) const { }
void FrequencyHeader::deserialize(std::ifstream& /*ifs*/) { }

} // namespace cobs

/******************************************************************************/
