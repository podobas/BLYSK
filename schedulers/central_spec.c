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

/* Description: The following the simplest of schedulers. It is a work-dealing algorithm where a 
                central queue is used to hold att released tasks. */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <omp.h>

#include <blysk_scheduler_api.h>
#include <blysk_GQueue.h>

#include <utils.h>

typedef struct {
    volatile u64 lt; /*< Last time */
    volatile u64 bt; /*< Thread blocked time */
    volatile u64 rt; /*< Running time */
    volatile u64 ns; /*< Number of times scheduled */
    volatile u64 nb; /*< Number of times blocked */
    volatile int blocked;
} TA_ALIGNED(64) ThreadLocal;

static ThreadLocal* tl;
static int nThreads;
static __thread int tid;

/* Declare the Central Queue as a GQueue (General-Queue) */
static GQueue Central_Queue;

static inline void updateTiming(u64 t, ThreadLocal* restrict tl) {
    u64 inc = t - tl->lt;
    tl->lt = t;
    if(tl->blocked) {
        tl->bt += inc;
    } else {
        tl->nb++;
        tl->rt += inc;
    }
}

/** Called when a team is created. The *icv structure contains team info */
size_t SCHEDULER_System_Init(blysk_icv *icv) {
    fprintf(stderr, "[Schedule] Central spec enabled...\n");
    nThreads = icv->n_Threads;
    GQueue_Init(&Central_Queue);
    tl = realloc(tl, icv->n_Threads * sizeof(*tl));
    tid = 0;
//    __sync_synchronize();
    return 0;
}



/** SCHEDULER_Thread_Init: Called when a thread has been added to the team. */
void SCHEDULER_Thread_Init(blysk_thread *thr) {
//    tid = __sync_fetch_and_add(&nThreads, 1);
    tid = thr->r_ID;
    tl[tid].lt = getTimeStamp();
    tl[tid].blocked = tid != 0;
    tl[tid].bt = 0;
    tl[tid].rt = 0;
    tl[tid].nb = 0;
    tl[tid].ns = 0;
    fprintf(stderr, "Hello from %d\n", tid);
}

void SCHEDULER_Thread_Exit(blysk_thread *__restrict thr) { }

void SCHEDULER_Exit(void) {
    u64 t = getTimeStamp();
    for(int tid = 0; tid != nThreads; tid++) {
        updateTiming(t, tl + tid);
        
        fprintf(stderr, "Thread %d waited for %f, scheduled: %" PRIu64 " blocked: %" PRIu64 " \n", 
                tid,
                100*(((double)tl[tid].bt)/(tl[tid].bt + tl[tid].rt)),
                tl[tid].ns,
                tl[tid].nb);
    }
    fprintf(stderr, "Goodbye from %d\n", tid);
    free(tl);
    tl = 0;
}

/** Called when a thread wants to release a task */
void SCHEDULER_Release(blysk_thread *thr, blysk_task *task) {
    GQueue_Enqueue_Front(&Central_Queue, task);
    //GQueue_Enqueue_Back ( Central_Queue , task);
}

static inline blysk_task* trackTime(blysk_task* restrict task) {
    if((task == 0) != tl[tid].blocked) {
        u64 t = getTimeStamp();
        updateTiming(t, tl + tid);
        tl[tid].blocked = (task == 0);
    }
    tl[tid].ns++;
    return task;
}

static inline blysk_task* myfetch() {
    return Central_Queue.__elements == 0 ? NULL : GQueue_Pop_Front(&Central_Queue);
}

/** Called when a thread has nothing to do */
blysk_task * SCHEDULER_Fetch(blysk_thread *thr) {
    blysk_task* t = trackTime(myfetch());
    return t;
}

/** Called when a thread is blocked and is looking for work */
blysk_task * SCHEDULER_Blocked_Fetch(blysk_thread *thr) {
    return SCHEDULER_Fetch(thr);
}
