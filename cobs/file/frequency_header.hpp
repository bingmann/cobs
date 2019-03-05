/*******************************************************************************
 * cobs/file/frequency_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_FREQUENCY_HEADER_HEADER
#define COBS_FILE_FREQUENCY_HEADER_HEADER

#include <cobs/file/header.hpp>

namespace cobs {

class FrequencyHeader
{
public:
    static const std::string magic_word;
    static const uint32_t version;
    static const std::string file_extension;

    void serialize(std::ofstream& ofs) const;
    void deserialize(std::ifstream& ifs);
};

} // namespace cobs

#endif // !COBS_FILE_FREQUENCY_HEADER_HEADER

/******************************************************************************/
