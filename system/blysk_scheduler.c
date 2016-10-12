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
#include <sys/syscall.h>
#include <linux/futex.h>

#include <blysk_public.h>
#include <blysk_dlsym.h>
#include <blysk_dep.h>
#include <blysk_task.h>
#include <blysk_icv.h>
#include <blysk_trace.h>
#include <spec.h>
#include <circular_queue.h>
#include <utils.h>

#include "blysk_dep_imp.h"



/* Scheduler is only loaded once, during boot-up */
__attribute((constructor))
void blysk__SCHEDULER_new() {
    blysk_icv *icv = &bpc;

    void *dll_handle;

    char *designated_scheduler = getenv("OMP_TASK_SCHEDULER");
    char extended_name[100];
    if (designated_scheduler == NULL) {
        fprintf(stderr, "\tOMP_TASK_SCHEDULER not set or not found. Using work-steal scheduler by default.\n");
        strcpy(extended_name, "lib_sched_worksteal.so");
    }
    else
        sprintf(extended_name, "lib_sched_%s.so", designated_scheduler);

    dll_handle = dlopen(extended_name, RTLD_LAZY);

    if (!dll_handle) {
        fprintf(stderr, "\t'%s' not found. Set LD_LIBRARY_PATH to point to the location of the libraries...\n", extended_name);
        abort();
    }

    // TODO use declspec or typeof operators to cast automatically
    icv->scheduler.scheduler_system_init = (size_t (*)(void*)) mydlsym(dll_handle, "SCHEDULER_System_Init", extended_name);
    icv->scheduler.scheduler_thread_init = (void (*)(void*)) mydlsym(dll_handle, "SCHEDULER_Thread_Init", extended_name);
    icv->scheduler.scheduler_release = (void (*)(void*, blysk_task*)) mydlsym(dll_handle, "SCHEDULER_Release", extended_name);
    icv->scheduler.scheduler_fetch = (blysk_task * (*)(void*)) mydlsym(dll_handle, "SCHEDULER_Fetch", extended_name);
    icv->scheduler.scheduler_blocked_fetch = (blysk_task * (*)(void*)) mydlsym(dll_handle, "SCHEDULER_Blocked_Fetch", extended_name);
    icv->scheduler.scheduler_thread_exit = (void (*)(void*)) mydlsym(dll_handle, "SCHEDULER_Thread_Exit", extended_name);
    icv->scheduler.scheduler_exit = mydlsym(dll_handle, "SCHEDULER_Exit", extended_name);


    /* Load other information */
    char *proc_bind = getenv("OMP_PROC_BIND");
    icv->bind_threads = proc_bind != NULL && (strcmp(proc_bind, "true") == 0 || strcmp(proc_bind, "TRUE") == 0);
}

static inline void runTaskArgs(blysk_task* __restrict new_task, blysk_task* __restrict current_task, void (*tsk)(void *), void* args) {
    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_TASK);
    blysk__THREAD_set_task(new_task);
#if defined(__TRACE)
    {
        blysk__TRACE_comm_record(new_task->__trace_releaser, new_task->__trace_release_time,
                blysk__THREAD_get_rid(), wall_time());
    }
#endif

    #if defined(__STAT_TASK)
       new_task->stat__cpu_id = blysk__THREAD_get_rid();
       new_task->stat__exec_cycles = rdtsc();
    #endif

    tsk(args);

    #if defined(__STAT_TASK)
       new_task->stat__exec_end_instant = rdtsc();
       new_task->stat__joins_at = new_task->_parent->stat__joins_counter;
    #endif


    handleSpec();
    blysk_task *new_task_parent = new_task->_parent;
    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_DEP_RELEASE);
    fetchAndAddCounter(&new_task_parent->_children, -1, RELEASE);

#if defined(__STAT_TASK) 
    blysk__THREAD_get()->tsc[1] = rdtsc();
#endif

    blysk__DEP_release(new_task);

#if defined(__STAT_TASK) 
    blysk__THREAD_get()->tsc[2] = rdtsc();
    new_task-> stat__dependency_resolution_time += blysk__THREAD_get()->tsc[2] - blysk__THREAD_get()->tsc[1];
    //test_dump_task ( new_task );
    blysk__GG_add_complete_task ( new_task);
#endif


    blysk__THREAD_set_task(current_task);
}

/** Run new_task to completion, then return to current_task. Updates the trace state, but does not return it. */
static inline int runTask(blysk_task* __restrict new_task, blysk_task* __restrict current_task) {
    if (new_task == NULL) {
        return 1;
    }
    runTaskArgs(new_task, current_task, new_task->tsk, getTaskArgs(new_task));
    return 0;
}

#if BLYSK_ENABLE_SPEC != 0

static inline void tabError(u32 error) {
    if((error & 1) != 0) {
        blysk__thread->specAbort++;
        return;
    }
    blysk__THREAD_get()->specConflict++;
}

static inline int specReturn(volatile unsigned int* adr, unsigned int val) {
    u32 error;
    AtomicBegin(fail, error);
    blysk__THREAD_get()->specAdr = adr;
    blysk__THREAD_get()->specVal = val;
    return 1;
fail:
    if((error & 1) != 0) {
        blysk__thread->specAbort++;
    }
//    blysk__THREAD_get()->specFail++;
    return 0;
}
#else
#define specReturn(adr, val) (0)
#endif // ENABLE_SPEC != 0
#if BLYSK_ENABLE_DATASPEC
/** Run a task speculatively */
static inline int specStart(blysk_task* current_task) {
    u32 error;
    blysk_task* t = cq_peek();
    if(t == 0) {
        return 0;
    }
    AtomicBegin(fail, error);
    blysk__THREAD_get()->specAdr = SPEC_TASK;
    runTask(t, current_task);
    return 1;
fail:
    if((error & 3) == 0) { // Transaction not caused by abort, and retry will not succeed
        cq_dequeue();
    } else {
        if ((error & 2) != 0)
            blysk__THREAD_get()->specAbort++;
        else
            blysk__THREAD_get()->specConflict++;
        cq_skip();
    }
//    blysk__THREAD_get()->specFail++;
    return 0;
}
#else
#define specStart(task) (0)
#endif

#if BLYSK_ENABLE_TRACK_BLOCK
#define trackBlocked(r) \
    u64 blockTimeStamp; int rOld = blysk__thread->rOld; \
    if(expectFalse(r != rOld)) { \
        blockTimeStamp = getTimeStamp();\
        blysk__thread->rOld=r;\
    }

#define tabBlocked() \
    if(expectFalse(r > rOld)) { /* Blocked begin */ \
        blysk__thread->blockedCycles -= blockTimeStamp; \
    } else if (expectFalse(r < blysk__thread->rOld)) { /* Blocked end */ \
        blysk__thread->blockedCycles += getTimeStamp(); \
    }
    
#define trackBlocked2(r) \
    u64 blockTimeStamp; int rOld = blysk__thread->rOld; blysk__thread->rOld = r; \
    if(expectFalse((r) > rOld)) { /* Blocked begin */ \
        blockTimeStamp = getTimeStamp();\
        blysk__thread->blockedCycles -= blockTimeStamp; \
    } else if (expectFalse((r) < rOld)) { /* Blocked end */ \
        blockTimeStamp = getTimeStamp();\
        blysk__thread->blockedCycles += blockTimeStamp; \
    }

#define finishBlock() \
    if(expectFalse(blysk__thread->rOld != 0)) { \
        blysk__thread->blockedCycles += getTimeStamp(); \
        blysk__thread->rOld = 0;\
    }


#define specBlock() \
    { \
        blysk__thread->blockedCycles += blockTimeStamp; \
        blysk__thread->rOld = 0;\
    }
#else
#define trackBlocked(r) ((void)0)
#define tabBlocked() ((void)0)
#define trackBlocked2(r) ((void)0)
#define finishBlock() ((void)0)
#define specBlock() ((void)0)
#endif


#if defined(__STAT_TASK) 

#define STAT_ENTER_TW \
    unsigned long long __tsc_enter_tw = rdtsc(); \
    unsigned long long __tsc_exit_tw; \
    current_task->stat__arrive_counter[ current_task-> stat__joins_counter] = rdtsc(); \
    if ( current_task-> stat__joins_counter > 15) {fprintf(stderr,"TOO MANY TASKWAIT\n"),abort();}

#define STAT_UPDATE_TW \
 if (new_task != NULL) \
    { \
      __tsc_exit_tw = rdtsc(); \
     current_task->  stat__overhead_cycles += (__tsc_exit_tw - __tsc_enter_tw); \
    }

#define STAT_RESUME_TW \
    if (new_task != NULL) \
    { unsigned long long tmp_1 = rdtsc(); \
     current_task->  stat__exec_cycles += (tmp_1 - __tsc_exit_tw); \
	__tsc_enter_tw = rdtsc();} 

#define STAT_FINISH_TW \
    __tsc_exit_tw = rdtsc(); \
    current_task->  stat__overhead_cycles += (__tsc_exit_tw - __tsc_enter_tw);  \
    current_task-> stat__joins_counter ++;

#else

#define STAT_ENTER_TW ;
#define STAT_UPDATE_TW ;
#define STAT_RESUE_TW ;
#define STAT_FINISH_TW ;
#define STAT_RESUME_TW  ;
#endif


/*
    TASK_taskwait
 */
__attribute((always_inline)) static inline int blysk__SCHEDULER_taskwait(void) {
    blysk_task *current_task = blysk__THREAD_get_task();
    handleSpec();


    STAT_ENTER_TW;

    bool tried = false;
    #if BLYSK_ENABLE_SPEC != 0
    blysk__thread->specPossible++;
    #endif
    /* No current task */
    if (current_task == NULL) return 0;
    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_BLOCK);
    blysk_icv *icv = blysk__THREAD_get_icv();
    unsigned activeChildren;
    while (0 != (activeChildren = __atomic_load_n(&current_task->_children.count, memory_order_relaxed))) {
        blysk_task *new_task = icv->scheduler.scheduler_blocked_fetch(blysk__THREAD_get());
        //blysk_task *new_task = SCHEDULER_Blocked_Fetch(blysk__THREAD_get());
        trackBlocked2(new_task == 0);


	STAT_UPDATE_TW;

        int r = runTask(new_task, current_task);

	STAT_RESUME_TW;


        // TODO, avoid code duplication
        if (r == 1 && activeChildren < 2) {  // Nothing to do :(
#if BLYSK_ENABLE_TRACK_BLOCK
            blockTimeStamp = getTimeStamp();
#endif
            if(specStart(current_task)) continue; // Speculatively execute a nearly ready task
            
            // Speculatively return, remember to test (current_task->_children == 0)
            if (specReturn(&current_task->_children.count, 0)) {
                specBlock();
                return 1;
            } else if (!tried) {
                #if BLYSK_ENABLE_SPEC != 0
                tried = true;
                blysk__thread->specFail++;
                #endif
            }
        }
        //~ tabBlocked();
        BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_BLOCK);
        compilerMemBarrier();
//        __sync_synchronize();
    }
    
    // TODO missing memory barrier
    finishBlock();
    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_TASK);

    STAT_FINISH_TW;

    return 0;
}


void blysk__ICV_barrier(void) {
    blysk_task *current_task = blysk__THREAD_get_task();
    handleSpec();
    blysk_icv *icv = blysk__THREAD_get_icv();
 
    int spec = blysk__SCHEDULER_taskwait();

    STAT_ENTER_TW;


#if BLYSK_ENABLE_SPEC != 0
    blysk__thread->specPossible++;
#endif
    
    unsigned int barrier_id = icv->icv_barrier.barrier_id;
    unsigned int thread_last;
    if (spec) {
        // TODO this is very unlikely to succeed. Should defer
        thread_last = icv->icv_barrier.barrier_waiting.count--; 
        handleSpec();
    } else {
        thread_last = fetchAndAddCounter(&icv->icv_barrier.barrier_waiting, -1, ACQ_REL);
//        thread_last = __sync_fetch_and_sub(&icv->icv_barrier.barrier_waiting, 1);
    }

    if (thread_last == 1) // We are done, let's tell the others
    {
        icv->icv_barrier.barrier_waiting.count = icv->n_Remaining;
//        __sync_synchronize();
        __atomic_store_n(&icv->icv_barrier.barrier_id, barrier_id + 1, RELEASE);
//        icv->icv_barrier.barrier_id++;
//        __sync_synchronize();

        /*Force everyone awake. */
        blysk__ICV_unlockAllFutex();
        return;
    }
    
    bool tried = false;
    do { // We have to wait, let's do something
        blysk_task *new_task = icv->scheduler.scheduler_fetch(blysk__THREAD_get());
        //blysk_task *new_task = SCHEDULER_Fetch(blysk__THREAD_get());
        trackBlocked2(new_task == 0);


	STAT_UPDATE_TW;


        int r = runTask(new_task, current_task);

	STAT_RESUME_TW;

        BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_IDLE);
        if (r == 1) { // Nothing to do :(
#if BLYSK_ENABLE_TRACK_BLOCK
            blockTimeStamp = getTimeStamp();
#endif
            if(specStart(current_task)) continue; // Speculatively execute a nearly ready task
            
            // Speculatively return, remember to test (icv->icv_barrier.barrier_id == barrier_id + 1)
            if (specReturn(&icv->icv_barrier.barrier_id, barrier_id + 1)) {
                specBlock();
                return;
            } else if (!tried) {
                #if BLYSK_ENABLE_SPEC != 0
                tried = true;
                blysk__thread->specFail++;
                #endif
            }
        }
        //~ tabBlocked();
#if BLYSK_ENABLE_ADAPT != 0
        /* Put the thread to sleep?*/
        u32 reqThrs = __atomic_load_n(&icv->adaptation.requestedThreads, RELAXED), tid = (u32)blysk__thread->r_ID;
        if(reqThrs <= tid) {
            fetchAndAddCounter(&icv->adaptation.runningThreads, -1, RELAXED);
            do {
//                fprintf(stderr, "Going to sleep %u on %p\n", tid, &icv->adaptation.requestedThreads);
                syscall(__NR_futex, &icv->adaptation.requestedThreads, FUTEX_WAIT, reqThrs, NULL, NULL, NULL);
//                fprintf(stderr, "Woke up %u\n", tid);
            } while(__atomic_load_n(&icv->adaptation.requestedThreads, ACQUIRE) <= tid);
            fetchAndAddCounter(&icv->adaptation.runningThreads, 1, RELAXED);
        }
//        if (icv->adaptation.adapt_online) {
//            if (icv->adapt_dorment[ blysk__THREAD_get()->r_ID] == 1) {
//                icv->adapt_sleep[ blysk__THREAD_get()->r_ID] = 1;
//                syscall(__NR_futex, &icv->adapt_dorment[ blysk__THREAD_get()->r_ID], FUTEX_WAIT, 1, NULL, NULL, NULL);
//            }
//
//            icv->adapt_sleep[ blysk__THREAD_get()->r_ID] = 0;
//        }
#endif
        compilerMemBarrier();
//        __sync_synchronize();
    } while (__atomic_load_n(&icv->icv_barrier.barrier_id, memory_order_relaxed) != barrier_id + 1 ||
            __atomic_load_n(&icv->icv_barrier.barrier_id, memory_order_acquire) != barrier_id + 1);
    finishBlock();
    /*Force everyone awake. */
    blysk__ICV_unlockAllFutex();

    STAT_FINISH_TW;


}

/* Some of these functions should be moved into ICV */

void GOMP_taskwait(void) {
    handleSpec();
    blysk__SCHEDULER_taskwait();
}


void BLYSK__barrier(void) {
    blysk__ICV_barrier();
}

void GOMP_barrier(void) {
    handleSpec();
    BLYSK__barrier();
}
