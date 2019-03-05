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
const uint32_t FrequencyHeader::version = 1;
const std::string FrequencyHeader::file_extension = ".freq.cobs";

void FrequencyHeader::serialize(std::ofstream& ofs) const {
    serialize_magic_begin(ofs, magic_word, version);

    serialize_magic_end(ofs, magic_word);
}

void FrequencyHeader::deserialize(std::ifstream& ifs) {
    deserialize_magic_begin(ifs, magic_word, version);

    deserialize_magic_end(ifs, magic_word);
}

} // namespace cobs

/******************************************************************************/
