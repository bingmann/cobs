/*******************************************************************************
 * cobs/query/classic_index/base.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_INDEX_BASE_HEADER
#define COBS_QUERY_CLASSIC_INDEX_BASE_HEADER
#pragma once

#include <cobs/file/classic_index_header.hpp>
#include <cobs/query/classic_base.hpp>
#include <cobs/util/timer.hpp>

namespace cobs::query::classic_index {

class base : public query::classic_base
{
protected:
    explicit base(const fs::path& path);
    uint64_t num_hashes() const override;
    uint64_t block_size() const override;
    uint64_t counts_size() const override;
    const std::vector<std::string>& file_names() const override;

    file::classic_index_header m_header;

public:
    virtual ~base() = default;
};

} // namespace cobs::query::classic_index

#endif // !COBS_QUERY_CLASSIC_INDEX_BASE_HEADER

/******************************************************************************/
