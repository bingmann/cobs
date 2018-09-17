/*******************************************************************************
 * cobs/query/classic_index/base.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/classic_index/base.hpp>

namespace cobs::query::classic_index {

base::base(const fs::path& path) : query::base<file::classic_index_header>(path) { }

uint64_t base::num_hashes() const {
    return m_header.num_hashes();
}

uint64_t base::block_size() const {
    return m_header.block_size();
}

uint64_t base::counts_size() const {
    return 8 * m_header.block_size();
}

} // namespace cobs::query::classic_index

/******************************************************************************/
