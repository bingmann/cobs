/*******************************************************************************
 * cobs/file/document_header.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_DOCUMENT_HEADER_HEADER
#define COBS_FILE_DOCUMENT_HEADER_HEADER

#include <cobs/file/header.hpp>

namespace cobs {

class DocumentHeader : public Header<DocumentHeader>
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
    DocumentHeader() = default;
    DocumentHeader(std::string name, uint32_t kmer_size);
    std::string name() const;
    uint32_t kmer_size() const;
};

} // namespace cobs

#endif // !COBS_FILE_DOCUMENT_HEADER_HEADER

/******************************************************************************/
