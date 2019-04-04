/*******************************************************************************
 * cobs/query/classic_base.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_BASE_HEADER
#define COBS_QUERY_CLASSIC_BASE_HEADER

#include <cobs/query/base.hpp>
#include <cobs/util/query.hpp>

#include <immintrin.h>

namespace cobs::query {

class classic_base : public base
{
private:
#ifdef NO_SIMD
    static const uint64_t m_expansions[16];
#else
    alignas(__m128i_u) static const uint16_t m_expansion[2048];
#endif
    void compute_counts(size_t hashes_size, uint16_t* counts, const char* rows,
                        size_t size);
    void aggregate_rows(size_t hashes_size, char* rows, size_t size);
    void create_hashes(std::vector<uint64_t>& hashes, const std::string& query);

protected:
    StreamPos stream_pos_;

    virtual void read_from_disk(const std::vector<size_t>& hashes, char* rows,
                                size_t begin, size_t size) = 0;

    virtual uint32_t term_size() const = 0;
    virtual uint8_t canonicalize() const = 0;
    virtual uint64_t row_size() const = 0;
    virtual uint64_t page_size() const = 0;
    virtual uint64_t num_hashes() const = 0;
    virtual uint64_t counts_size() const = 0;
    virtual const std::vector<std::string>& file_names() const = 0;

public:
    void search(const std::string& query,
                std::vector<std::pair<uint16_t, std::string> >& result,
                size_t num_results = 0) final;
};

} // namespace cobs::query

#endif // !COBS_QUERY_CLASSIC_BASE_HEADER

/******************************************************************************/
