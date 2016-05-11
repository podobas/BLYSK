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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <blysk_public.h>
#include <blysk_scheduler.h>
#include <blysk_thread.h>
#include <blysk_icv.h>
#include <blysk_task.h>
#include <adapt.h>
#include <pageAlloc.h>
#include <slr_lock.h>

//TODO: fix the bpc.active part.

blysk_icv bpc;
unsigned int blysk_thread__online = 0;

/* Per thread private variables */
__thread blysk_thread * blysk__thread = NULL;

blysk_thread **blysk__booted_threads;

static inline void allocMe(uptr r_id, blysk_icv* icv) {
    uptr size = sizeof(blysk_thread);
    blysk_thread* thr = (blysk_thread*) pageAlloc(&size);
    if (thr == NULL) {
        fprintf(stderr, "Failed to allocate thread space\n");
        abort();
    }
    if (icv->bind_threads == 1) {
        cpu_set_t set;
        CPU_ZERO(&set); 
        CPU_SET(r_id, &set); // Run on cpu r_id
        sched_setaffinity(0, sizeof (cpu_set_t), &set);
    }
    thr->r_ID = r_id;
#if BLYSK_ENABLE_TRACK_BLOCK
    thr->blockedCycles = 0;
    thr->rOld = 0;
#endif
#if BLYSK_ENABLE_SPEC != 0
    thr->specAdr = 0;
    thr->specFail = 0;
    thr->specPossible = 0;
    thr->specSuccess = 0;
    thr->specAbort = 0;
#endif
    blysk__thread = thr;
    blysk__booted_threads[r_id] = thr;
}

static inline void initMe(uptr r_id, blysk_icv* icv) {
#if BLYSK_ENABLE_FANOUT_CONTROL != 0
    thr->fanout = 1;
#endif
    blysk_thread* __restrict thr = blysk__thread;
    thr->implicit_task = allocTask(0, 0, 0);
    if (thr->implicit_task == NULL) {
        fprintf(stderr, "Failed to allocate implicit task\n");
        abort();
    }
    thr->implicit_task->_parent = NULL;
    thr->implicit_task->_children = (Counter) {0};
    thr->implicit_task->_dependencies = 0;
    thr->implicit_task->_udependencies = (Counter) {0};
    thr->implicit_task->__dep_manager = NULL;
    thr->implicit_task->FPN[0] = NULL;
    thr->implicit_task->FPN[1] = NULL;
#if BLYSK_ENABLE_DATASPEC != 0    
    thr->cq_back = 0;
    thr->cq_front = 0;
    for(unsigned i = 0; i != CQ_CAP; i++) {
        thr->cq_elem[i] = 0;
    }
#endif
    
    blysk__THREAD_set_icv(icv);
}

/** Barrier called by threads when they enter a parallel section */
static inline void startBarrier(blysk_icv* icv) {
    unsigned int my_id = fetchAndAddCounter(&icv->n_Expected, -1, ACQ_REL);
    if(icv->opt_nowait != 1) { // We need to wait for the other threads before starting
        /* If my_id == 1 then Im the last thread. Release the icv and enter the parallel region */
        if (my_id == 1) {
            fetchAndAddCounter(&icv->icv_release, 1, RELEASE); // TODO Why are we using a counter??
            return;
        }
        while(__atomic_load_n(&icv->icv_release.count, ACQUIRE) == 0) { }
    }
}

void blysk__THREAD_icv_barrier(Counter* bar) {
    unsigned int rem = fetchAndAddCounter(bar, -1, ACQ_REL);
    
    while(__atomic_load_n(&bar->count, ACQUIRE) != 0) { }
    
//rem_loop:
//    __sync_synchronize();
//    if (*bar != 0) goto rem_loop;
}


#if BLYSK_ENABLE_SPEC_THREADS
#include <spec.h>
#include "blysk_dep_imp.h"

static void* blysk__THREAD_afk_spec(void* vThr) {
    unsigned pTid = (uptr) vThr;
    unsigned tid = pTid + bpc.n_Threads;
    blysk_thread* thr = bpc.booted_threads[pTid];
    allocMe(tid, &bpc);
    initMe(blysk__thread->r_ID, &bpc);
    //   fprintf(stderr,"[%d] Entered afk loop\n",blysk__thread->r_ID);
    /* Enter afk loop */
    while (1) {
        for(unsigned i = 0; i != CQ_CAP; i++) {
            blysk_task* t = __atomic_load_n(&blysk__thread->cq_elem[i], __ATOMIC_ACQUIRE);
            if(t != 0) {
                u32 error;
                AtomicBegin(fail, error);
                t->tsk(getTaskArgs(t));
                blysk_task* task = blysk__THREAD_get_task();
                if(*t->self != t || // Allowed to segfault, it will abort
                        0 /* invalidAdr(task->self)*/) { 
                    AtomicAbort(2);
                }
                blysk_task *new_task_parent = t->_parent;
                fetchAndAddCounter(&new_task_parent->_children, -1, RELEASE);
            //    __sync_fetch_and_sub(&new_task_parent->_children, 1);
                blysk__DEP_release(t);
                __atomic_compare_exchange_n(&thr->cq_elem[i], &t, 0, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
                continue;
            fail: {}
//                if(e) {
//                    
//                }
            }
        }
    }
    BI_UNREACHABLE;
    return 0;
}
#endif

/* Enters a thread into the ICV */
void blysk__THREAD_enter(blysk_icv *icv) {
    //fprintf(stderr,"[%d] entered ICV\n",blysk__THREAD_get_rid());
    initMe(blysk__thread->r_ID, &bpc);
    icv->scheduler.scheduler_thread_init(&blysk__thread);
    //SCHEDULER_Thread_Init(blysk__thread);
    startBarrier(&bpc);
#if BLYSK_ENABLE_SPEC_THREADS
    {
        pthread_t thread;
        int r = pthread_create(&thread, NULL, &blysk__THREAD_afk_spec, (void*) ((uintptr_t)((unsigned)blysk__thread->r_ID)));
        sassert(r == 0);
    }
    #endif
    icv->f_Entry(icv->f_Arg); // Enter the parallel section
    blysk__ICV_barrier();
    blysk__THREAD_set_icv(NULL);
    icv->scheduler.scheduler_thread_exit(blysk__thread);
    //SCHEDULER_Thread_Exit(blysk__thread);
    if(blysk__thread->r_ID == 0) {
        icv->active = 0;
    }
    blysk__THREAD_icv_barrier(&icv->icv_complete_barrier[1]);
}

/*--------------------------------
        Thread No-Parallel Loop	
  -------------------------------- */
void* blysk__THREAD_afk(void* thrNum) {
    uptr tid = (uptr)thrNum;
    allocMe(tid, &bpc);

    //   fprintf(stderr,"[%d] Entered afk loop\n",blysk__thread->r_ID);
    /* Enter afk loop */
    while (1) {
        if(bpc.active != 0) {
            blysk__THREAD_enter(&bpc);
        }
        usleep(250);
    }
}

/*
   Boot a number of threads...
 */
void blysk__THREAD_boot(blysk_icv *icv) {
    unsigned int n_thread = icv->n_Expected.count;
    if (blysk_thread__online < n_thread) {
        blysk__booted_threads = (blysk_thread **) realloc(blysk__booted_threads, 
#if BLYSK_ENABLE_SPEC_THREADS
        2*
#endif
        n_thread * sizeof (blysk_thread *));
        icv->booted_threads = blysk__booted_threads;
        if (blysk__booted_threads == NULL) {
            fprintf(stderr, "Failed to allocate threads\n");
            abort();
        }
    } 
    if(blysk_thread__online == 0) {
        /* Boot calling thread */
        allocMe(0, icv);
        blysk_thread__online = 1;
    }
    if(blysk_thread__online == n_thread) {
        return;
    }
    /* Boot external threads */
    unsigned i = blysk_thread__online;
    blysk_thread__online = n_thread;
    
    for(; i < n_thread; i++) {
        pthread_t thread;
        int r = pthread_create(&thread, NULL, &blysk__THREAD_afk, (void*)(uptr)i);
        sassert(r == 0);
    }
}

void
GOMP_parallel_start (void (*fn) (void *), void *data, unsigned num_threads)
{
#if defined(__STAT_TASK)
    blysk__GG_init ();
#endif

    char* omp_num_threads;
    if(num_threads == 0 && // If number of threads not specified, and
            ((omp_num_threads = getenv("OMP_NUM_THREADS")) == 0 || // OMP_NUM_THREADS not specified, or
            (num_threads = atoi(omp_num_threads)) == 0)) { // OMP_NUM_THREADS invalid
        num_threads = omp_get_num_procs(); // Just use the number of processors
    }
    fprintf(stderr, "Booting with %d threads...\n", num_threads);

    blysk__ICV_new(&bpc, num_threads, fn, data, 0); // Initialize ICV
#if defined(__TRACE)
	blysk__TRACE_init(icv->n_Expected);
#endif
    
    blysk__THREAD_boot(&bpc);
    blysk__THREAD_enter(&bpc); //Lets call the parallel region
}

void 
GOMP_parallel_end (void) {
    //  All threads have run and entered the end barrier  
//    blysk__ADAPTATION_clean(&bpc);
    blysk__ICV_unlockAllFutex();

#if BLYSK_ENABLE_TRACK_BLOCK || BLYSK_ENABLE_SPEC || BLYSK_ENABLE_DATASPEC
    for (unsigned i = 0; i != blysk_thread__online; i++) {
        blysk_thread* t = blysk__booted_threads[i];
        fprintf(stderr, "%u:", i);
#endif
#if BLYSK_ENABLE_SPEC || BLYSK_ENABLE_DATASPEC
        fprintf(stderr, " specSuccess %d  specPossible %d  specFail %d  specAbort %d  specConflict %d ", t->specSuccess, t->specPossible, t->specFail, t->specAbort, t->specConflict);
#endif
#if BLYSK_ENABLE_TRACK_BLOCK != 0
        fprintf(stderr, " specBlocked %" PRIu64 " ", t->blockedCycles);
#endif
#if BLYSK_ENABLE_TRACK_BLOCK || BLYSK_ENABLE_SPEC || BLYSK_ENABLE_DATASPEC
        fputs("\n", stderr);
    }
#endif
    bpc.scheduler.scheduler_exit();

#if defined(__TRACE)
    blysk__TRACE_done();
#endif

#if defined(__STAT_TASK)
    blysk__GG_exit ();
#endif
}

void GOMP_parallel(void (*fn)(void *), void *data, unsigned num_threads,
        unsigned int flags) {
//    BLYSK__parallel(num_threads, 1, fn, data);
    GOMP_parallel_start(fn, data, num_threads); // Create, 
    //blysk__THREAD_enter(&bpc); // Not needed anymore. Moved to parallel_start
    GOMP_parallel_end(); // End the parallel section
}
