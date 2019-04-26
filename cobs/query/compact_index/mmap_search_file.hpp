/*******************************************************************************
 * cobs/query/compact_index/mmap_search_file.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_QUERY_COMPACT_INDEX_MMAP_SEARCH_FILE_HEADER
#define COBS_QUERY_COMPACT_INDEX_MMAP_SEARCH_FILE_HEADER

#include <cobs/query/compact_index/search_file.hpp>

namespace cobs {

class CompactIndexMMapSearchFile : public CompactIndexSearchFile
{
private:
    MMapHandle handle_;
    std::vector<uint8_t*> data_;

protected:
    void read_from_disk(const std::vector<size_t>& hashes, uint8_t* rows,
                        size_t begin, size_t size, size_t buffer_size) override;

public:
    explicit CompactIndexMMapSearchFile(const fs::path& path);
    ~CompactIndexMMapSearchFile();
};

} // namespace cobs

#endif // !COBS_QUERY_COMPACT_INDEX_MMAP_SEARCH_FILE_HEADER

/******************************************************************************/
