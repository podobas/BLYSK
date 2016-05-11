#pragma once

#include <blysk_config.h>
#if BLYSK_ENABLE_SPEC || BLYSK_ENABLE_DATASPEC

#include <blysk_public.h>
#include <utils.h>

static inline void handleSpec() {
    blysk_thread* t = blysk__THREAD_get();
    if (expectTrue(t->specAdr == 0)) {
        return;
    }
#if BLYSK_ENABLE_DATASPEC
    if(t->specAdr == SPEC_TASK) {
        blysk_task* task = blysk__THREAD_get_task();
        if(*task->self != task || // Allowed to segfault, it will abort
                0 /* invalidAdr(task->self)*/) { 
            AtomicAbort(2);
        }
        *task->self = 0;
    } else 
#endif
        if (expectFalse(t->specVal != *t->specAdr)) {
            AtomicAbort(1);
        }
    
    AtomicEnd;

    t->specSuccess++;
    t->specAdr = 0;
}
#else
#define handleSpec()
#endif // ENABLE_SPEC != 0


