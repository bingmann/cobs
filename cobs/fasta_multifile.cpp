/*******************************************************************************
 * cobs/fasta_multifile.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/fasta_multifile.hpp>

#include <thread>

namespace cobs {

ThreadObjectLRUSet<std::ifstream> FastaMultifile::lru_set_ {
    std::thread::hardware_concurrency()* 4
};

FastaIndexCache FastaMultifile::cache_;

} // namespace cobs

/******************************************************************************/
