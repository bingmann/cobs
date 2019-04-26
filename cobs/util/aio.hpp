/*******************************************************************************
 * cobs/util/aio.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_AIO_HEADER
#define COBS_UTIL_AIO_HEADER

#include <linux/aio_abi.h>
#include <time.h>

namespace cobs {

int io_setup(unsigned nr, aio_context_t* ctxp);
int io_destroy(aio_context_t ctx);
int io_submit(aio_context_t ctx, long nr, iocb** iocbpp);
int io_getevents(aio_context_t ctx, long min_nr, long max_nr, io_event* events, timespec* timeout);

} // namespace cobs

#endif // !COBS_UTIL_AIO_HEADER

/******************************************************************************/
