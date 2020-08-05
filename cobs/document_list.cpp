/*******************************************************************************
 * cobs/document_list.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/document_list.hpp>

#include <tlx/string/to_lower.hpp>

namespace cobs {

FileType StringToFileType(std::string& s) {
    tlx::to_lower(&s);
    if (s == "any" || s == "*")
        return FileType::Any;
    if (s == "text" || s == "txt")
        return FileType::Text;
    if (s == "cortex" || s == "ctx")
        return FileType::Cortex;
    if (s == "cobs" || s == "cobs_doc")
        return FileType::KMerBuffer;
    if (s == "fasta")
        return FileType::Fasta;
    if (s == "fastq")
        return FileType::Fastq;
    if (s == "list")
        return FileType::List;
    die("Unknown file type " << s);
}

} // namespace cobs

/******************************************************************************/
