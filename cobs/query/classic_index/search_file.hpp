/*******************************************************************************
 * cobs/query/classic_index/search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_CLASSIC_INDEX_SEARCH_FILE_HEADER
#define COBS_QUERY_CLASSIC_INDEX_SEARCH_FILE_HEADER

#include <cobs/file/classic_index_header.hpp>
#include <cobs/query/index_file.hpp>

namespace cobs {

class ClassicIndexSearchFile : public IndexSearchFile
{
protected:
    explicit ClassicIndexSearchFile(const fs::path& path);

    uint32_t term_size() const final { return header_.term_size_; }
    uint8_t canonicalize() const final { return header_.canonicalize_; }
    uint64_t num_hashes() const final { return header_.num_hashes_; }
    uint64_t row_size() const final { return header_.row_size(); }
    uint64_t page_size() const final { return 1; }
    uint64_t counts_size() const final;
    const std::vector<std::string>& file_names() const override {
        return header_.file_names_;
    }

    ClassicIndexHeader header_;

public:
    virtual ~ClassicIndexSearchFile() = default;
};

} // namespace cobs

#endif // !COBS_QUERY_CLASSIC_INDEX_SEARCH_FILE_HEADER

/******************************************************************************/
