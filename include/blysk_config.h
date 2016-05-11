#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(__ARMEL__) || defined(__ARM_ARCH_5T__) || defined(__ARM_PCS) || defined(__ARM_ARCH) || defined(__ARM_EABI__)
#define onArm 1
#define onX86 0
#define onSparc 0
#elif defined(sparc) || defined(__sparc__) || defined(__sparc) || defined(__sparc_v8__)
#define onArm 0
#define onX86 0
#define onSparc 1
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(__amd64__) || defined(i386) || defined(__i386) || defined(__i386__)
#define onArm 0
#define onX86 1
#define onSparc 0
#else
#error "Unsupported architecture (try removing this line, it might work)"
#endif

/** Whether threads should track how much time they spend blocked. */
#ifndef BLYSK_ENABLE_TRACK_BLOCK
#define BLYSK_ENABLE_TRACK_BLOCK (0)
#endif

    
/** Whether blysk should use speculation. */
#ifndef BLYSK_ENABLE_SPEC
#define BLYSK_ENABLE_SPEC (0)
#endif

/** Whether blysk should use speculation for data dependencies. */
#ifndef BLYSK_ENABLE_DATASPEC
#define BLYSK_ENABLE_DATASPEC (0)
#endif

/** Whether each thread should have a speculative thread.
 *  The speculative thread speculatively executes its owners blocked tasks. */
#ifndef BLYSK_ENABLE_SPEC_THREADS
#define BLYSK_ENABLE_SPEC_THREADS (0)
#endif

/** Whether blysk should use lock-elision (specifically slr). */
#ifndef BLYSK_ENABLE_SLR
#define BLYSK_ENABLE_SLR (0)
#endif

/** Whether blysk support bookmarks, to quickly recognize dependencies- */
#define BLYSK_ENABLE_BOOKMARKS (0)

/** Whether blysk supports libpappadapt */
#ifndef BLYSK_ENABLE_ADAPT 
#define BLYSK_ENABLE_ADAPT (0)
#endif

/** Whether blysk uses a trie to dependencies (1), or a linked list (0) */
#define BLYSK_ENABLE_DEP_TRIE (1)

/** Determines the memory allocator used by BLYSK. 
 *  At 0, BLYSK uses malloc and free.
 *  At 1, each thread will defer freeing 56 320 bytes, at most.
 *  At 2, each thread will defer freeing up to 11 * (PAGE_SIZE / 64) objects,
 *  and the memory will never be reclaimed to the OS.
 *  Settings higher than 0 may increase performance significantly. */
#if onSparc
#define BLYSK_ENABLE_FAST_MALLOC (1)
#else
#define BLYSK_ENABLE_FAST_MALLOC (2)
#endif

#define BLYSK_ENABLE_FANOUT_CONTROL (0)

/** Whether blysk is allowed to use mmap for memory allocation. 
 *  When enabled, most memory allocations are still malloc */
#define BLYSK_ENABLE_MMAP (1)

#if onSparc
#define BLYSK_DEP_STRIPED_LOCK (1)
#else
//#define BLYSK_DEP_GLOBAL_LOCK (1)
//#define BLYSK_DEP_STRIPED_LOCK (1)
#define BLYSK_DEP_CAS_STRIPED_LOCK (1)
//#define BLYSK_DEP_SLR_LOCK (1)
#endif

#if BLYSK_DEP_GLOBAL_LOCK + BLYSK_DEP_STRIPED_LOCK + BLYSK_DEP_CAS_STRIPED_LOCK + BLYSK_DEP_SLR_LOCK != 1
#error "Must specify exactly ONE lock mechanism for the dependency manager"
#endif
    
#if !onX86 && (BLYSK_ENABLE_SLR || BLYSK_ENABLE_SPEC || BLYSK_DEP_SLR_LOCK || BLYSK_ENABLE_SPEC_THREADS)
#error "BLYSK only supports speculative execution on x86"
#endif

#if BLYSK_ENABLE_SPEC_THREADS && !BLYSK_ENABLE_DATASPEC
#error "Speculative threads require speculation on data dependencies"
#endif
//#define SOFTWARE_CACHE 1

#ifdef __cplusplus 
}
#endif
