/*******************************************************************************
 * cobs/util/serialization.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/util/serialization.hpp>

namespace cobs {

stream_metadata get_stream_metadata(std::ifstream& ifs) {
    std::streamoff curr_pos = ifs.tellg();
    ifs.seekg(0, std::ios::end);
    std::streamoff end_pos = ifs.tellg();
    ifs.seekg(curr_pos, std::ios::beg);
    assert(ifs.good());
    assert(curr_pos >= 0);
    assert(end_pos >= 0);
    assert(end_pos >= curr_pos);
    return {
               (uint64_t)curr_pos, (uint64_t)end_pos
    };
}

void serialize(std::ostream& /*ost*/) { }
void deserialize(std::istream& /*ist*/) { }

} // namespace cobs

/******************************************************************************/
