#define _GNU_SOURCE   /* syscall() is not POSIX */

#include <cobs/util/aio.hpp>

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <iostream>
#include <vector>


namespace cobs::query {
    int io_setup(unsigned nr, aio_context_t* ctxp) {
        return syscall(__NR_io_setup, nr, ctxp);
    }

    int io_destroy(aio_context_t ctx) {
        return syscall(__NR_io_destroy, ctx);
    }

    int io_submit(aio_context_t ctx, long nr, iocb** iocbpp) {
        return syscall(__NR_io_submit, ctx, nr, iocbpp);
    }

    int io_getevents(aio_context_t ctx, long min_nr, long max_nr, io_event* events, timespec* timeout) {
        return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
    }
}
