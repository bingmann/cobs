/*******************************************************************************
 * cobs/query/index_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_INDEX_FILE_HEADER
#define COBS_QUERY_INDEX_FILE_HEADER

#include <cobs/util/query.hpp>

#include <immintrin.h>

namespace cobs {

class IndexSearchFile
{
public:
    StreamPos stream_pos_;

    virtual void read_from_disk(
        const std::vector<size_t>& hashes, uint8_t* rows,
        size_t begin, size_t size) = 0;

    virtual uint32_t term_size() const = 0;
    virtual uint8_t canonicalize() const = 0;
    virtual uint64_t row_size() const = 0;
    virtual uint64_t page_size() const = 0;
    virtual uint64_t num_hashes() const = 0;
    virtual uint64_t counts_size() const = 0;
    virtual const std::vector<std::string>& file_names() const = 0;
};

} // namespace cobs

#endif // !COBS_QUERY_INDEX_FILE_HEADER

/******************************************************************************/
