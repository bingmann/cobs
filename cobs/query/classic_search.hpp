/*******************************************************************************
 * cobs/query/classic_search.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_SEARCH_HEADER
#define COBS_QUERY_CLASSIC_SEARCH_HEADER

#include <cobs/query/index_file.hpp>
#include <cobs/query/search.hpp>
#include <cobs/util/query.hpp>

namespace cobs::query {

class ClassicSearch : public Search
{
private:
    static const uint64_t s_expansion[16];
    alignas(16) static const uint16_t s_expansion_128[2048];

    void compute_counts(size_t hashes_size, uint16_t* counts, const uint8_t* rows,
                        size_t size);
    void aggregate_rows(size_t hashes_size, uint8_t* rows, size_t size);
    void create_hashes(std::vector<uint64_t>& hashes, const std::string& query);

public:
    ClassicSearch(IndexFile& index_file)
        : index_file_(index_file) { }

    void search(
        const std::string& query,
        std::vector<std::pair<uint16_t, std::string> >& result,
        double threshold = 0.0, size_t num_results = 0) final;

protected:
    //! reference to index file query object to retrieve data
    IndexFile& index_file_;
};

} // namespace cobs::query

#endif // !COBS_QUERY_CLASSIC_SEARCH_HEADER

/******************************************************************************/
