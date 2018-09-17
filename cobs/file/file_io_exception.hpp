/*******************************************************************************
 * cobs/file/file_io_exception.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FILE_FILE_IO_EXCEPTION_HEADER
#define COBS_FILE_FILE_IO_EXCEPTION_HEADER
#pragma once

namespace cobs::file {

class file_io_exception : public std::runtime_error
{
private:
    std::string m_msg;

public:
    explicit file_io_exception(const std::string& msg) : std::runtime_error(msg), m_msg(msg) { }

    virtual const char * what() const noexcept {
        return m_msg.c_str();
    }

    std::string& message() {
        return m_msg;
    }
};

} // namespace cobs::file

#endif // !COBS_FILE_FILE_IO_EXCEPTION_HEADER

/******************************************************************************/
