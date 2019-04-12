/*******************************************************************************
 * cobs/util/zip_stream_fwd.hpp
 *
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_ZIP_STREAM_FWD_HEADER
#define COBS_UTIL_ZIP_STREAM_FWD_HEADER

#include <string>

namespace cobs {

template <class CharT,
          class Traits = std::char_traits<CharT> >
class basic_zip_ostream;

template <class CharT,
          class Traits = std::char_traits<CharT> >
class basic_zip_istream;

//! A typedef for basic_zip_ostream<char>
using zip_ostream = basic_zip_ostream<char>;
//! A typedef for basic_zip_istream<char>
using zip_istream = basic_zip_istream<char>;

} // namespace cobs

#endif // !COBS_UTIL_ZIP_STREAM_FWD_HEADER

/******************************************************************************/
