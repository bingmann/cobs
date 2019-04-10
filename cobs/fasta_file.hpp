/*******************************************************************************
 * cobs/fasta_file.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FASTA_FILE_HEADER
#define COBS_FASTA_FILE_HEADER

#include <cstring>
#include <string>
#include <vector>

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/string_view.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

namespace cobs {

class FastaFile
{
public:
    FastaFile(std::string path) : path_(path) {
        is_.open(path);
        die_unless(is_.good());

        std::string line;
        std::getline(is_, line);
        die_unless(is_.good());

        if (line.empty() || (line[0] != '>' && line[0] != ';'))
            die("FastaFile: file does not start with > or ;");

        while (data_begin_ = is_.tellg(), std::getline(is_, line)) {
            if (line[0] != '>' && line[0] != ';')
                break;
        }
    }

    //! return estimated size of a fasta document
    size_t size() {
        is_.clear();
        is_.seekg(0, std::ios::end);
        return is_.tellg() - data_begin_;
    }

    template <typename Callback>
    void process_terms(size_t term_size, Callback callback) {
        is_.clear();
        is_.seekg(data_begin_);
        die_unless(is_.good());

        std::string data, line;

        while (std::getline(is_, line)) {
            if (line[0] == '>' || line[0] == ';')
                continue;

            data += line;
            if (data.empty())
                continue;

            for (size_t i = 0; i + term_size <= data.size(); ++i) {
                callback(string_view(data.data() + i, term_size));
            }
            data.erase(0, data.size() - term_size + 1);
        }
    }

private:
    //! file stream
    std::ifstream is_;
    //! path
    std::string path_;
    //! length of header
    std::streamoff data_begin_;
};

} // namespace cobs

#endif // !COBS_FASTA_FILE_HEADER

/******************************************************************************/
