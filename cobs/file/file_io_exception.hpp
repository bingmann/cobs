/*******************************************************************************
 * cobs/file/file_io_exception.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_FILE_IO_EXCEPTION_HEADER
#define COBS_FILE_FILE_IO_EXCEPTION_HEADER

#include <stdexcept>

namespace cobs {

class FileIOException : public std::runtime_error
{
private:
    std::string msg_;

public:
    explicit FileIOException(const std::string& msg)
        : std::runtime_error(msg), msg_(msg) { }

    const char * what() const noexcept override {
        return msg_.c_str();
    }

    std::string& message() {
        return msg_;
    }
};

} // namespace cobs

#endif // !COBS_FILE_FILE_IO_EXCEPTION_HEADER

/******************************************************************************/
