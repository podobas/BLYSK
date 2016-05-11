#pragma once

#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <blysk_config.h>
#include <features.h>

#ifdef __cplusplus 
extern "C" {
#endif

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)

typedef struct blysk_icv_t blysk_icv; /*< Blysk's Internal Control Variables */
typedef struct blysk_thread_t blysk_thread ; /*< A associated with the blysk_icv. Each thread has some thread local variables. */
typedef struct blysk_task_t blysk_task;

extern __thread blysk_thread * blysk__thread __attribute((externally_visible)); /*< The currently running thread */
extern blysk_thread **blysk__booted_threads __attribute((externally_visible)); /*< The running threads */
extern unsigned int blysk_thread__online __attribute((externally_visible)); /*< The number of running threads */

typedef struct blysk_dep_data_t blysk_dep_data;
typedef struct blysk_dep_object_t blysk_dep_object;
typedef struct blysk_bit_table_t blysk_bit_table;

/** OpenMP style taskwait:
 *  Wait for all tasks spawned from this parallel section. */
void GOMP_taskwait(void) 
__attribute((externally_visible))
;

#if __GNUC_PREREQ(5,1)
void
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	   long arg_size, long arg_align, bool if_clause, unsigned flags,
	   void **depend, int priority);
#elif __GNUC_PREREQ(4,9)
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
        long arg_size, long arg_align, bool if_clause, unsigned flags,
        void **depend __attribute((nonnull(1), externally_visible));
#elif __GNUC_PREREQ(4,8)
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, char if_clause,
           unsigned flags __attribute__((unused)));
#else
	#error Unsupport GCC version
#endif




void
GOMP_taskloop (void (*) (void *), void *, void (*) (void *, void *),
	       long, long arg_align, unsigned flags,
	       unsigned long , int ,
	       long , long, long );


/** OpenMP style barrier, implemented in two steps: 
 *  1. Wait for all tasks spawned from this parallel section.
 *  2. Wait for all threads to reach the barrier. */
void GOMP_barrier(void) __attribute((externally_visible));

/** Called by the master thread to create (not execute) a parallel section.
 *  All threads but the master may execute the parallel section. */
void GOMP_parallel_start (void (*fn) (void *), void *data, unsigned num_threads) __attribute((nonnull(1), externally_visible));

/** Called by the master thread at the end of a parallel section */
void 
GOMP_parallel_end (void) __attribute((externally_visible));

/** Called by the master thread to create, execute, and end a parallel section */
void GOMP_parallel(void (*fn)(void *), void *data, unsigned num_threads,
        unsigned int flags) __attribute((nonnull(1), externally_visible));
        
char GOMP_single_start(void) __attribute((externally_visible));

void GOMP_critical_start(void) __attribute((externally_visible));

void GOMP_critical_end(void) __attribute((externally_visible));

void GOMP_critical_name_start (void ** __restrict pptr) __attribute((externally_visible));

void GOMP_critical_name_end (void ** __restrict pptr) __attribute((externally_visible));

/** Test-And-Set lock. Zero initialized. */
typedef struct {
    unsigned l;
} TASLock;

/** Atomic counter. Zero initialized */
#if onArm || onX86
typedef struct Counter_t {
    unsigned count;
} Counter;
#else
typedef struct Counter_t {
    unsigned count;
    TASLock lock;
} Counter;
#endif

typedef struct {
    size_t (*scheduler_system_init)(void *); /*< Called by master thread before any thread starts */
    void (*scheduler_thread_init)(void *); /*< Called by each thread before it starts */
    void (*scheduler_thread_exit)(void *); /*< Called by each thread at exit */
    void (*scheduler_exit)(void); /*< Called by master thread after all threads exit */
    void (*scheduler_release)(void *, blysk_task *);
    blysk_task * (*scheduler_fetch)(void *);
    blysk_task * (*scheduler_blocked_fetch)(void *);
} Scheduler;

typedef struct {
    pthread_t h_ID;

    void* (*adapt_main)(void *);
    void (*adapt_kill)(void);
    volatile uint32_t requestedThreads; // TODO align to cache line
    Counter runningThreads; // TODO align to cache line
    unsigned char adapt_online;
} Adaptation;

typedef struct {
    Counter barrier_waiting;
    unsigned int barrier_id;
} blysk_barrier;

typedef struct blysk_icv_t {
    Scheduler scheduler;
    Adaptation adaptation;
    void (*f_Entry)(void*); /* Function Entry */
    void *f_Arg; /* Function Argument */
    blysk_thread** booted_threads;
//    int *adapt_dorment;
//    unsigned char *adapt_sleep;
    size_t tlSize;
    unsigned int n_Threads;
    unsigned int n_Remaining; /* How many thread completed the context */
    Counter n_Expected; /* How many threads are expected to come here */
    Counter icv_release; /* Release team */
    unsigned int opt_single;
    /* Options */
    unsigned char opt_nowait; /* No need to wait for others */
    unsigned char bind_threads; /*< 1 if the treads are bound to individual cpus, otherwise the threads are unbound. */

    Counter icv_complete_barrier[2];
    blysk_barrier icv_barrier;
    unsigned int opt_barrier; /* ICV barrier */
    volatile int active;


#if defined(__STAT_TASK)
    unsigned long long stat__start_time;
#endif

} blysk_icv;


typedef struct blysk_dep_data_t {
    blysk_dep_object* o; /*< Pointer to dependency object in the linked lists */
    // TODO, why do we track m? debugging purposes? it is never read!
    signed char m; /*< Type of dependency: DEP_READ, DEP_WRITE, DEP_READWRITE or DEP_UNUSED. Maybe use enum. DEP_UNUSED is tested but never used */
#if defined(__TRACE)
    unsigned char r;
#endif
} blysk_dep_data;

typedef struct blysk_task_t blysk_task;

#define CQ_CAP (4)

typedef struct blysk_thread_t {
//    pthread_t p_ID; /* Pthread (or other) ID */
    blysk_icv *icv;
    blysk_task *implicit_task;

    void *metadata; /* Meta-data used by e.g. the scheduler */
    int r_ID; /* Resident ID */
#if BLYSK_ENABLE_FANOUT_CONTROL != 0
    int fanout;
#endif
#if BLYSK_ENABLE_TRACK_BLOCK != 0
    uint64_t blockedCycles;
    int rOld;
#endif
#if BLYSK_ENABLE_SPEC != 0 || BLYSK_ENABLE_DATASPEC
    volatile unsigned int* specAdr;
    unsigned int specVal;
    int specSuccess; /*< The number of taskwaits and barriers where one transaction succeeded */
    int specFail; /*< The number of taskwaits and barriers where at least one transaction failed */
    int specPossible; /*< The number of taskwaits and barriers */
    int specAbort; /*< The number of aborted transactions */
    int specConflict; /*< The number of transactions which failed due to conflicts */
#endif
#if BLYSK_ENABLE_DATASPEC
#define SPEC_TASK ((volatile unsigned int*) 1)
    unsigned char cq_back; 
    unsigned char cq_front;
    blysk_task* cq_elem[CQ_CAP];
#endif


#if defined(__STAT_TASK)
    unsigned long long tsc[6]; /* Timestamp counters */
#endif

//    char PADDING[64];
} blysk_thread;


/* Added: CORE_Profile counters */

typedef struct blysk_task_t {
    /* Number of children of the current task */
    Counter _children;
    Counter _udependencies; /*< Number of unsatisfied dependencies. Heavily updated with atomics and locks, maybe store together with children. */
#if BLYSK_ENABLE_DATASPEC != 0
    struct blysk_task_t** self; /*< Pointer to the task in the task queue. Used for speculation */
#endif
    char PADDING[64 - sizeof (Counter) * 2 - (BLYSK_ENABLE_DATASPEC ? sizeof(void*) : 0)];
    unsigned long long __dep_LOCK_mask; /*< Bitmask of the depdency tries this task uses */

    void (*tsk)(void *);
    //  void *args;

    #if defined(__STAT_TASK)
     unsigned long stat__task;
     unsigned long stat__parent;
     unsigned long stat__joins_at;
     unsigned long stat__joins_counter;
     unsigned long stat__arrive_counter[16];
     unsigned long stat__cpu_id;
     unsigned long stat__cpu_id_create;
     unsigned int  stat__child_number;
     unsigned long stat__num_children;
     unsigned long stat__exec_cycles;
     unsigned long long stat__creation_cycles;
     unsigned long long stat__overhead_cycles;
     unsigned long stat__queue_size;
     unsigned long long stat__create_instant;
     unsigned long long stat__exec_end_instant;
     
     unsigned int stat__cpu_id_release;
     unsigned long long stat__release_instant;
     unsigned long long stat__dependency_resolution_time;
    #endif

    /* Parent to the task */
    blysk_task *_parent;
    void *FPN[2]; /*< What is this, next/prev pointers? */

    blysk_bit_table *__dep_manager; /*< Tracks the children task's dependencies */
    void* args;

    size_t size; /*< Size of this task, including dependencies and arguments */
    unsigned int _dependencies; /*< Total number of dependencies. */

#if defined(__TRACE)
    unsigned long __trace_release_time;
    int __trace_releaser;
#endif

#ifndef NDEBUG
    unsigned int __task_id;
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    /** The data dependencies of this task.
     *  The dependencies are stored after the normal task data. 
     *  The task arguments are stored after the dependencies. */
    blysk_dep_data dependency[0];
#pragma GCC diagnostic pop
} blysk_task;

// Thread local getters/setters

__attribute((const)) static inline blysk_thread * blysk__THREAD_get(void)  {
    return blysk__thread;
}

/** Return the current task */
static inline blysk_task * blysk__THREAD_get_task(void) {
    return blysk__thread->implicit_task;
}

/** Set the current task */
static inline void blysk__THREAD_set_task(blysk_task * curtsk) {
    blysk__thread->implicit_task = curtsk;
}

/** Return the thread id, the r_ID, from a thread */
static inline unsigned int blysk__THREAD_get_rid(void) {
    return blysk__thread->r_ID;
}

/** Return the current ICV of a thread... */
static inline blysk_icv * blysk__THREAD_get_icv(void) {
    return blysk__thread -> icv;
}

/** Set the ICV for a thread */
static inline void blysk__THREAD_set_icv(blysk_icv *icv) {
    blysk__thread -> icv = icv;
}





#if defined(__i386__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#else
  #error "Grain-Graph support only for x86-64 or i386 architectures"
#endif


#ifdef __cplusplus 
}
#endif

