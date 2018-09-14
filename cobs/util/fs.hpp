/*******************************************************************************
 * cobs/util/fs.hpp
 *
 *
 ******************************************************************************/

#ifndef COBS_UTIL_FS_HEADER
#define COBS_UTIL_FS_HEADER

#if __cplusplus >= 201703L

#include <experimental/filesystem>

namespace cobs {

namespace fs = std::experimental::filesystem;
using std::error_code;

} // namespace cobs

#else

#include <boost/filesystem.hpp>

namespace cobs {

namespace fs = boost::filesystem;
using boost::system::error_code;

} // namespace cobs

#endif

#endif // !COBS_UTIL_FS_HEADER

/******************************************************************************/
