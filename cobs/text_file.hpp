/*******************************************************************************
 * cobs/text_file.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_TEXT_FILE_HEADER
#define COBS_TEXT_FILE_HEADER

#include <cstring>
#include <string>
#include <vector>

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/string_view.hpp>

#include <tlx/die.hpp>

namespace cobs {

class TextFile
{
public:
    TextFile(std::string path) : path_(path) {
        is_.open(path);
        die_unless(is_.good());
    }

    //! return size of a text document
    size_t size() {
        is_.clear();
        is_.seekg(0, std::ios::end);
        return is_.tellg();
    }

    //! return number of q-grams in document
    size_t num_terms(size_t q) {
        size_t n = size();
        return n < q ? 0 : n - q + 1;
    }

    template <typename Callback>
    void process_terms(size_t term_size, Callback callback) {
        is_.clear();
        is_.seekg(0);

        char buffer[64 * 1024];
        size_t pos = 0;

        while (!is_.eof()) {
            is_.read(buffer + pos, sizeof(buffer) - pos);
            size_t wb = is_.gcount();

            for (size_t i = 0; i + term_size <= pos + wb; ++i) {
                callback(string_view(buffer + i, term_size));
            }

            if (wb + 1 < term_size)
                break;

            std::copy(buffer + wb - term_size + 1, buffer + wb,
                      buffer);
            pos = term_size - 1;
        }
    }

private:
    //! file stream
    std::ifstream is_;
    //! path
    std::string path_;
};

} // namespace cobs

#endif // !COBS_TEXT_FILE_HEADER

/******************************************************************************/
