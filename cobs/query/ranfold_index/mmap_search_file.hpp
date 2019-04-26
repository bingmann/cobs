/*******************************************************************************
 * cobs/query/ranfold_index/mmap_search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_RANFOLD_INDEX_MMAP_SEARCH_FILE_HEADER
#define COBS_QUERY_RANFOLD_INDEX_MMAP_SEARCH_FILE_HEADER

#include <cobs/query/ranfold_index/search_file.hpp>

namespace cobs {

class RanfoldIndexMMapSearchFile : public RanfoldIndexSearchFile
{
private:
    MMapHandle handle_;
    uint8_t* data_;

protected:
    void read_from_disk(const std::vector<size_t>& hashes, char* rows);

public:
    explicit RanfoldIndexMMapSearchFile(const fs::path& path);
    ~RanfoldIndexMMapSearchFile();
};

} // namespace cobs

#endif // !COBS_QUERY_RANFOLD_INDEX_MMAP_SEARCH_FILE_HEADER

/******************************************************************************/
