/*******************************************************************************
 * cobs/fasta_file.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/fasta_file.hpp>

#include <thread>

namespace cobs {

ThreadObjectLRUSet<std::ifstream> FastaFile::lru_set_ {
    std::thread::hardware_concurrency()* 4
};

FastaIndexCache FastaFile::cache_;

} // namespace cobs

/******************************************************************************/
