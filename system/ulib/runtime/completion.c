// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <runtime/completion.h>

#include <limits.h>
#include <magenta/syscalls.h>
#include <system/atomic.h>

enum {
    UNSIGNALED = 0,
    SIGNALED = 1,
};

mx_status_t mxr_completion_wait(mxr_completion_t* completion, mx_time_t timeout) {
    // TODO(kulakowski): With a little more state (a waiters count),
    // this could optimistically spin before entering the kernel.

    int* futex = &completion->futex;

    for (;;) {
        int current_value = atomic_load(futex);
        if (current_value == SIGNALED) {
            return NO_ERROR;
        }
        switch (mx_futex_wait(futex, current_value, timeout)) {
        case NO_ERROR:
            continue;
        case ERR_BUSY:
            // If we get ERR_BUSY, the value of the futex changed between
            // our load and the wait. This could only have happened if we
            // were signaled.
            return NO_ERROR;
        case ERR_TIMED_OUT:
            return ERR_TIMED_OUT;
        case ERR_INVALID_ARGS:
        default:
            __builtin_trap();
        }
    }
}

void mxr_completion_signal(mxr_completion_t* completion) {
    int* futex = &completion->futex;
    atomic_store(futex, SIGNALED);
    mx_futex_wake(futex, UINT32_MAX);
}

void mxr_completion_reset(mxr_completion_t* completion) {
    atomic_store(&completion->futex, UNSIGNALED);
}
