#pragma once

#include <linux/aio_abi.h>
#include <time.h>

namespace cobs::query {
    int io_setup(unsigned nr, aio_context_t* ctxp);
    int io_destroy(aio_context_t ctx);
    int io_submit(aio_context_t ctx, long nr, iocb** iocbpp);
    int io_getevents(aio_context_t ctx, long min_nr, long max_nr, io_event* events, timespec* timeout);
}
