#pragma once

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


#ifdef __cplusplus
extern "C" {
#endif

typedef enum omp_proc_bind_t {
    omp_proc_bind_false = 0,
    omp_proc_bind_true = 1,
    omp_proc_bind_master = 2,
    omp_proc_bind_close = 3,
    omp_proc_bind_spread = 4
} omp_proc_bind_t;

typedef enum omp_sched_t {
    omp_sched_static = 1,
    omp_sched_dynamic = 2,
    omp_sched_guided = 3,
    omp_sched_auto = 4
} omp_sched_t;
    
/* OMP API functions */

/* Context functions */

/** Sets the number of threads to be used for
subsequent parallel regions that do not specify a num_threads clause, by setting the
value of the first element of the nthreads-var ICV of the current task. */
void omp_set_num_threads(int num_threads) __attribute((externally_visible));

/** Returns the number of threads in the current
team. */
int omp_get_num_threads(void) __attribute((externally_visible, pure));

/** Returns an upper bound on the number of
threads that could be used to form a new team if a parallel region without a
num_threads clause were encountered after execution returns from this routine. */
int omp_get_max_threads(void) __attribute((externally_visible, const));

/** Returns the thread number, within the current
team, of the calling thread. */
int omp_get_thread_num(void) __attribute((externally_visible, pure));

/** Returns the number of processors available to the
program. */
int omp_get_num_procs(void) __attribute((externally_visible, pure));

/* Returns true if the call to the routine is enclosed by an
active parallel region; otherwise, it returns false. */
int omp_in_parallel(void) __attribute((externally_visible, pure));

/** Enables or disables dynamic adjustment of the
number of threads available for the execution of subsequent parallel regions by
setting the value of the dyn-var ICV. */
void omp_set_dynamic(int dynamic_threads) __attribute((externally_visible));

/** Returns the value of the dyn-var ICV, which
determines whether dynamic adjustment of the number of threads is enabled or disabled. */
int omp_get_dynamic(void) __attribute((externally_visible, pure));

/** Returns the value of the cancel-var ICV, which
controls the behavior of the cancel construct and cancellation points. */
int omp_get_cancellation(void) __attribute((externally_visible, pure));

/** Enables or disables nested parallelism, by setting the
nest-var ICV. */
void omp_set_nested(int nested) __attribute((externally_visible));

/** Returns the value of the nest-var ICV, which
determines if nested parallelism is enabled or disabled. */
int omp_get_nested(void) __attribute((externally_visible, pure));

/** Sets the schedule that is applied when runtime
is used as schedule kind, by setting the value of the run-sched-var ICV. */
void omp_set_schedule(omp_sched_t kind, int modifier) __attribute((externally_visible));

/** Sets *kid to the schedule that is applied when the
runtime schedule is used. */
void omp_get_schedule(omp_sched_t *__restrict kind, int * __restrict modifier) __attribute((nonnull(1,2), externally_visible));

/** Returns the maximum number of OpenMP
threads available to the program. */
int omp_get_thread_limit(void) __attribute((externally_visible, const));

/** Limits the number of nested active
parallel regions, by setting the max-active-levels-var ICV. */
void omp_set_max_active_levels(int max_levels) __attribute((externally_visible));

/** Returns the value of the max-activelevels-
var ICV, which determines the maximum number of nested active parallel
regions. */
int omp_get_max_active_levels(void) __attribute((externally_visible, pure));

/** Returns the number of nested parallel regions
enclosing the task that contains the call. */
int omp_get_level(void) __attribute((externally_visible, pure));

/** Returns, for a given nested level of
the current thread, the thread number of the ancestor or the current thread. */
int omp_get_ancestor_thread_num(void) __attribute((externally_visible, pure));

/** Returns, for a given nested level of the current
thread, the size of the thread team to which the ancestor or the current thread belongs. */
int omp_get_team_size(void) __attribute((externally_visible, pure));

/** Returns the number of nested, active
parallel regions enclosing the task that contains the call. */
int omp_get_active_level(void) __attribute((externally_visible, pure));

/** Returns true if the routine is executed in a final task
region; otherwise, it returns false. */
int omp_in_final(void) __attribute((externally_visible, pure));

/** Returns the thread affinity policy to be used for the
next most closely nested parallel region. */
omp_proc_bind_t omp_get_proc_bind(void) __attribute((externally_visible));

/** Assigns the value of the default-device-var
ICV, which determines the default device number. */
void omp_set_default_device(int device_num) __attribute((externally_visible));

/** Returns the value of the default-device-var
ICV, which determines the default device number. */
int omp_get_default_device(void) __attribute((externally_visible));

/**Returns the number of target devices. */
int omp_get_num_devices(void) __attribute((externally_visible, const));

/** Returns the number of teams in the current teams
region. */
int omp_get_num_teams(void) __attribute((externally_visible));

/** Returns the team number of the calling thread. */
int omp_get_team_num(void) __attribute((externally_visible, pure));


/* Timing functions */

/** Returns elapsed wall clock time in seconds. */
double omp_get_wtime(void) __attribute((externally_visible));

/** Returns the precision of the timer used by omp_get_wtime.*/
double omp_get_wtick(void) __attribute((externally_visible)); 

typedef __attribute((aligned(64))) struct {char cacheLine[64];} omp_lock_t;
void omp_init_lock(omp_lock_t *lock) __attribute((nonnull(1), externally_visible));
void omp_unset_lock(omp_lock_t *lock) __attribute((nonnull(1), externally_visible));
void omp_set_lock(omp_lock_t *lock) __attribute((nonnull(1), externally_visible));
void omp_destroy_lock(omp_lock_t *lock) __attribute((nonnull(1), externally_visible));
int omp_test_lock(omp_lock_t *lock) __attribute((nonnull(1), externally_visible));

/* Internal functions for atomics */

#ifdef __cplusplus
}
#endif

