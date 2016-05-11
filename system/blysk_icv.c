/* 
Copyright (c) 2013, Artur Podobas
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Artur Podobas nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Artur Podobas BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

//#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <blysk_icv.h>

#include <blysk_thread.h>
#include <blysk_scheduler.h>
#include <blysk_trace.h>

#include <adapt.h>

void blysk__ICV_new(blysk_icv * restrict bpc, unsigned Expected_Threads, void(*f_Entry)(void*), void *f_Arg, unsigned char opt_nowait) {
    bpc->adaptation.runningThreads = (Counter) {Expected_Threads};
    bpc->adaptation.requestedThreads = Expected_Threads;
    bpc->n_Expected = (Counter) { Expected_Threads };
    bpc->f_Entry = f_Entry;
    bpc->f_Arg = f_Arg;
    bpc->n_Remaining = Expected_Threads;
    bpc->n_Threads = Expected_Threads;
    //   bpc->lock = 0;

    /* Allocate dorment holder. Reset to '0' */
//     TODO allocate in one block
//    bpc->adapt_dorment = (int *) malloc(sizeof (int) * Expected_Threads);
//    memset(bpc->adapt_dorment, 0, Expected_Threads * sizeof (int));
//    bpc->adapt_sleep = (unsigned char *) malloc(sizeof (int) * Expected_Threads);
//    memset(bpc->adapt_sleep, 0, Expected_Threads * sizeof (int));

    bpc->opt_nowait = opt_nowait;
    bpc->tlSize = 0;
    bpc->icv_release = (Counter) {0};
    bpc->opt_single = 1;
    bpc->opt_barrier = Expected_Threads;

    /* Two of the barriers that are used in the end of the parallel region */
    bpc->icv_complete_barrier[0] = (Counter) {Expected_Threads}; // TODO only the second one is used!!
    bpc->icv_complete_barrier[1] = (Counter) {Expected_Threads};

    bpc->icv_barrier.barrier_waiting = (Counter) {Expected_Threads};
    bpc->icv_barrier.barrier_id = 0;

//    blysk__SCHEDULER_new(bpc);
    SCHEDULER_System_Init(bpc);
//    blysk__ADAPTATION_new(bpc);
    bpc->active = 1;

#if defined(__STAT_TASK)
    bpc->stat__start_time = rdtsc();
#endif
}

char blysk__ICV_single(void) {
//    blysk_icv *cur = blysk__THREAD_get_icv();
    return __sync_lock_test_and_set(&bpc.opt_single, 0);
}

/* ICV Functions */
unsigned int blysk__ICV_threads(void) {
//    blysk_icv *cur = blysk__THREAD_get_icv();
    return bpc.n_Remaining;
}

#include <stdio.h>
#include <utils.h>

/* Force all threads in an icv to wake up */
void blysk__ICV_unlockAllFutex() {
#if BLYSK_ENABLE_ADAPT != 0
//    fputs(stderr, "BLYSK Waking threads\n");
    __atomic_store_n(&bpc.adaptation.requestedThreads, -1, RELEASE);
    compilerMemBarrier();
    syscall(__NR_futex, &bpc.adaptation.requestedThreads, FUTEX_WAKE, bpc.n_Threads, NULL, NULL, NULL);
    compilerMemBarrier();
#if 0
    blysk_icv *icv = (blysk_icv *) _icv;
    unsigned int i;
    for (i = 0; i < icv->n_Threads; i++) {
        icv->adapt_dorment[i] = 0;
        syscall(__NR_futex, &icv->adapt_dorment[ i ], FUTEX_WAKE, 1, NULL, NULL, NULL);
    }
#endif
#endif
}
