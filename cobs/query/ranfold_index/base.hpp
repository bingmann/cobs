/*******************************************************************************
 * cobs/query/ranfold_index/base.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_RANFOLD_INDEX_BASE_HEADER
#define COBS_QUERY_RANFOLD_INDEX_BASE_HEADER

#include <cobs/file/ranfold_index_header.hpp>
#include <cobs/query/classic_base.hpp>

namespace cobs::query::ranfold_index {

class base : public query::base
{
protected:
    explicit base(const fs::path& path);

    file::ranfold_index_header m_header;
    stream_metadata m_smd;

public:
    virtual ~base() = default;

    void search(const std::string& query, uint32_t kmer_size,
                std::vector<std::pair<uint16_t, std::string> >& result,
                size_t num_results = 0) final;
};

} // namespace cobs::query::ranfold_index

#endif // !COBS_QUERY_RANFOLD_INDEX_BASE_HEADER

/******************************************************************************/
