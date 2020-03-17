/*******************************************************************************
 * cobs/query/compact_index/search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_COMPACT_INDEX_SEARCH_FILE_HEADER
#define COBS_QUERY_COMPACT_INDEX_SEARCH_FILE_HEADER

#include <cobs/file/compact_index_header.hpp>
#include <cobs/query/index_file.hpp>

namespace cobs {

class CompactIndexSearchFile : public IndexSearchFile
{
protected:
    size_t num_hashes_;
    size_t row_size_;
    explicit CompactIndexSearchFile(const fs::path& path);

    uint32_t term_size() const final { return header_.term_size_; }
    uint8_t canonicalize() const final { return header_.canonicalize_; }
    uint64_t num_hashes() const final { return num_hashes_; }
    uint64_t page_size() const final { return header_.page_size_; }
    uint64_t row_size() const final { return row_size_; }
    uint64_t counts_size() const final;

    const std::vector<std::string>& file_names() const final {
        return header_.file_names_;
    }

    CompactIndexHeader header_;

public:
    virtual ~CompactIndexSearchFile() = default;
};

} // namespace cobs

#endif // !COBS_QUERY_COMPACT_INDEX_SEARCH_FILE_HEADER

/******************************************************************************/
