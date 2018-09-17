/*******************************************************************************
 * cobs/file/sample_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_SAMPLE_HEADER_HEADER
#define COBS_FILE_SAMPLE_HEADER_HEADER
#pragma once

#include <cobs/file/header.hpp>

namespace cobs::file {

class sample_header : public header<sample_header>
{
private:
    std::string m_name;
    uint32_t m_kmer_size;

protected:
    void serialize(std::ofstream& ofs) const override;
    void deserialize(std::ifstream& ifs) override;

public:
    static const std::string magic_word;
    static const std::string file_extension;
    sample_header() = default;
    sample_header(std::string name, uint32_t kmer_size);
    std::string name() const;
    uint32_t kmer_size() const;
};

} // namespace cobs::file

#endif // !COBS_FILE_SAMPLE_HEADER_HEADER

/******************************************************************************/
