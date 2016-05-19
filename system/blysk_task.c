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
#include <string.h>
#include <pthread.h>

#include <blysk_public.h>

#include <blysk_task.h>
#include <blysk_dep.h>
#include <blysk_trace.h>
#include <blysk_memory.h>
#include <blysk_malloc.h>
#include <circular_queue.h>
#include <utils.h>
#include <spec.h>
#include <slr_lock.h>
#include <blysk_transfer.h>

#include "blysk_transfer_imp.h" // Mostly implements bookmarks??

#include "blysk_dep_imp.h" // Implements the dependency manager and its synchronization


/// Task creation

#ifndef NDEBUG
static Counter task_ids = {1};
static inline void trackTask(blysk_task* new_task) {
    new_task -> __task_id = fetchAndAddCounter(&task_ids, 1, RELAXED);
}
#else
#define trackTask(new_task) ((void)0)
#endif

static Counter uniqueTask_ids = {1};

/* -----------------------------------------------------------------------------------------------------------------------------------------------------------------------
   Function: blysk__create_task
   Description: Creates and returns a new task that is initialized. */

static inline blysk_task* blysk__create_task(blysk_task* parent, void (*func)(void *), void *arg, void (*cpyfn) (void *, void *), u32 argsize, u32 arg_align, int nDeps) {
    blysk_task *new_task = allocTask(nDeps, argsize, arg_align);
    if (nDeps != 0 && parent->__dep_manager == NULL)
        blysk__DEP_init(parent);
    new_task->tsk = func;
    new_task->_dependencies = nDeps;
    new_task->_udependencies = (Counter) {(unsigned)nDeps};
    if(expectFalse(cpyfn != 0)) {
        cpyfn(getTaskArgs(new_task), arg);
    } else {
        memcpy(getTaskArgs(new_task), arg, argsize);
    }
    new_task->_parent = parent;
    new_task->_children = (Counter) {0};
    new_task->__dep_manager = NULL;
    new_task->FPN[0] = NULL;
    new_task->FPN[1] = NULL;

    new_task->Type = UNIT;
    trackTask(new_task);


    /* Clean this stuff up */
   #if defined(__STAT_TASK)
     new_task-> stat__task = fetchAndAddCounter(&uniqueTask_ids, 1, RELAXED);
     new_task-> stat__parent = parent != NULL  ? parent->stat__task : 0;
     new_task-> stat__create_instant = rdtsc();
     new_task-> stat__child_number = parent->stat__num_children++;
     new_task-> stat__cpu_id_create = blysk__THREAD_get_rid();
     new_task-> stat__num_children = 0;
   
     /* Set Others to default */
     new_task-> stat__cpu_id = -1;
     new_task-> stat__num_children = 0;
     new_task-> stat__exec_cycles = 0;
     new_task-> stat__creation_cycles = 0;
     new_task-> stat__overhead_cycles = 0;
     new_task-> stat__queue_size = 0;
     new_task-> stat__exec_end_instant = -1;    
     new_task-> stat__cpu_id_release = -1;
     new_task-> stat__release_instant= -1;
     new_task-> stat__dependency_resolution_time = -1;
     new_task-> stat__joins_at = -1;
     new_task-> stat__joins_counter = 0;
    #endif


    return new_task;
}

/* -----------------------------------------------------------------------------------------------------------------------------------------------------------------------
   Function: blysk__create_brood_Task
   Description: Creates a brood task and returns the task initialized */
blysk_task *blysk__create_brood_task (blysk_task *parent , void (*func)(void *), void *data , long start, long end)
{
	blysk_task *new_task = allocTask (0 , 0,  0);
	new_task->tsk = func;
	new_task->args = data;
	new_task->_dependencies = 0;
	new_task->_udependencies = (Counter) {0};

	if (expectTrue(parent == NULL)) 
	{
	  fetchAndAddCounter(&blysk__THREAD_get_task()->_parent->_children, 1, RELAXED);
 	  new_task->_parent = blysk__THREAD_get_task()->_parent;
	}
	else 
	{
	  fetchAndAddCounter(&parent->_children, 1, RELAXED);
	  new_task->_parent = parent;
	}


	new_task->__dep_manager = NULL;
	new_task->FPN[0] = NULL;
	new_task->FPN[1] = NULL;
	new_task->Type = BROOD;
	new_task->iter_space[0] = start;
	new_task->iter_space[1] = end;
	trackTask(new_task);
	new_task->creator = blysk__THREAD_get_rid();
	return new_task;	
}





/* Two new types of task : iteration task and divide and conquert task */

/**  Submits a "simple" task. A simple task is one which does not have any special functionality.
               Special functionality in this case is "depend" or "final". */
static inline void blysk__TASK_submit_simple(void (*func)(void *), void *arg,  void (*cpyfn) (void *, void *), u32 argsize, u32 arg_align) {
    blysk_task *parent = blysk__THREAD_get_task();
    blysk_task *new_task = blysk__create_task(parent, func, arg, cpyfn, argsize, arg_align, 0);
#if 0
    if (parent == NULL) { /* No parent? No parallel region! */
        func(arg);
        return;
    }
#endif

    int old = fetchAndAddCounter(&parent->_children, 1, RELAXED); // The scheduler release operation is also a release

/* Profile the creation time */
#if defined(__STAT_TASK)
    blysk__THREAD_get()->tsc[1] = rdtsc();
    parent-> stat__creation_cycles += blysk__THREAD_get()->tsc[1] - blysk__THREAD_get()->tsc[0];
#endif

    blysk_icv *icv = blysk__THREAD_get_icv();
    icv->scheduler.scheduler_release(blysk__THREAD_get(), new_task);
    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_TASK);

/* Profile the extra overheads asociated with scheduling */
#if defined(__STAT_TASK) 
    blysk__THREAD_get()->tsc[2] = rdtsc();
    parent-> stat__overhead_cycles += blysk__THREAD_get()->tsc[2] - blysk__THREAD_get()->tsc[1];
#endif

}

/** Submit a complex task with dependencies
  FIXME!!!? What do you mean FIXME? */
static inline void blysk__TASK_submit_complex_bookmark(void (*func)(void *), void *arg,  void (*cpyfn) (void *, void *), u32 argsize, u32 arg_align, char **_indep, int _nindep, char **_outdep, int _noutdep, char **_inoutdep, int _ninoutdep, char* bookmark) {
    blysk_task *parent = blysk__THREAD_get_task();
    assume(parent != 0);
    int nDeps = _nindep + _ninoutdep + _noutdep;
    assume(nDeps != 0);
    blysk_task *new_task = blysk__create_task(parent, func, arg,  cpyfn, argsize, arg_align, nDeps);
    int old = fetchAndAddCounter(&parent->_children, 1, RELAXED); // Either the DEP_solve or the scheduler_release call is a release operation
    /* Submit the new task to the dependency manager */
    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_DEP_SUBMIT);


/* Profile the creation time */
#if defined(__STAT_TASK)
    blysk__THREAD_get()->tsc[1] = rdtsc();
    parent-> stat__creation_cycles += blysk__THREAD_get()->tsc[1] - blysk__THREAD_get()->tsc[0];
#endif


/* Solve any deendencies*/
    char foundDeps = blysk__DEP_solve(new_task, _indep, _nindep,
            _outdep, _noutdep,
            _inoutdep, _ninoutdep, bookmark);


/* Profile the dependecy resoluion time */
#if defined(__STAT_TASK) 
    blysk__THREAD_get()->tsc[2] = rdtsc();
    new_task-> stat__dependency_resolution_time = blysk__THREAD_get()->tsc[2] - blysk__THREAD_get()->tsc[1];
#endif


/* If there are no outstanding dependencies, then we can schedule the task*/
    if (foundDeps == 0) {
        blysk_icv *icv = blysk__THREAD_get_icv();
        icv->scheduler.scheduler_release(blysk__THREAD_get(), new_task);
    }

/* Profile the extra overheads asociated with scheduling */
#if defined(__STAT_TASK)
    blysk__THREAD_get()->tsc[3] = rdtsc();
    parent-> stat__overhead_cycles += blysk__THREAD_get()->tsc[3] - blysk__THREAD_get()->tsc[2];
#endif

    BLYSK_TRACE(blysk__THREAD_get_rid(), STATE_TASK);
}




#if __GNUC_PREREQ(5,1)
void
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	   long arg_size, long arg_align, bool if_clause, unsigned flags,
	   void **depend, int priority)
{

  #if defined(__STAT_TASK)
      blysk__THREAD_get()->tsc[0] = rdtsc();
  #endif

    handleSpec();
    /* The representation of what GCC generates and what BlyskCC generates are no compatible.
       Here we transform from GCC to BlyskCC versioning 
       NOTE: GCC does not differ between out and inout. This can be a problem later. */
    if (depend == NULL) {
  #if BLYSK_ENABLE_FANOUT_CONTROL != 0
        if(expectTrue(blysk__thread->fanout != 0)) {
            blysk__TASK_submit_simple(fn, data, cpyfn, arg_size, arg_align);
        } else {
            fn(data);
        }
  #else // BLYSK_ENABLE_FANOUT_CONTROL == 0
        blysk__TASK_submit_simple(fn, data, cpyfn, arg_size, arg_align);
  #endif
        return;
        
    }
    // TODO do we actually want to have support for the if_clause? 
    unsigned int num_depend = (uintptr_t) depend[0];
    unsigned int num_inout = (uintptr_t) depend[1];
    blysk__TASK_submit_complex_bookmark(fn, data, cpyfn, arg_size, arg_align, (char**) (depend + 2 + num_inout), num_depend - num_inout, (char**) depend + 2, num_inout, NULL, 0, 0);


  #if defined(__STAT_TASK)
     blysk__THREAD_get()->tsc[4] = rdtsc();
  #endif
}

#elif __GNUC_PREREQ(4,9)
  void
  GOMP_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
          long arg_size, long arg_align, bool if_clause, unsigned flags,
          void **depend) {

  #if defined(__STAT_TASK)
      blysk__THREAD_get()->tsc[0] = rdtsc();
  #endif


    handleSpec();
    /* The representation of what GCC generates and what BlyskCC generates are no compatible.
       Here we transform from GCC to BlyskCC versioning 
       NOTE: GCC does not differ between out and inout. This can be a problem later. */
    if (depend == NULL) {
  #if BLYSK_ENABLE_FANOUT_CONTROL != 0
        if(expectTrue(blysk__thread->fanout != 0)) {
            blysk__TASK_submit_simple(fn, data, cpyfn, arg_size, arg_align);
        } else {
            fn(data);
        }
  #else // BLYSK_ENABLE_FANOUT_CONTROL == 0
        blysk__TASK_submit_simple(fn, data, cpyfn, arg_size, arg_align);
  #endif
        return;
        
    }
    // TODO do we actually want to have support for the if_clause? 
    unsigned int num_depend = (uintptr_t) depend[0];
    unsigned int num_inout = (uintptr_t) depend[1];
    blysk__TASK_submit_complex_bookmark(fn, data, cpyfn, arg_size, arg_align, (char**) (depend + 2 + num_inout), num_depend - num_inout, (char**) depend + 2, num_inout, NULL, 0, 0);


  #if defined(__STAT_TASK)
     blysk__THREAD_get()->tsc[4] = rdtsc();
  #endif
  }

#elif __GNUC_PREREQ(4,8)
  void
  GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, char if_clause,
           unsigned flags __attribute__((unused)))
  {
  #if defined(__STAT_TASK)
      blysk__THREAD_get()->tsc[0] = rdtsc();
  #endif

    handleSpec();
    blysk__TASK_submit_simple(fn, data, cpyfn, arg_size, arg_align);


  #if defined(__STAT_TASK)
     blysk__THREAD_get()->tsc[4] = rdtsc();
  #endif
  }
#endif


