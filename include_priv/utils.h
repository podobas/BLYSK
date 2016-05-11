#pragma once

#include <stdint.h>
#include <sys/time.h>
#include <blysk_config.h>

// Common typedefs, could also stem from include file
#define expectFalse(c) __builtin_expect((c), 0)
#define expectTrue(c) __builtin_expect((c), 1)
#define BI_UNREACHABLE __builtin_unreachable()
#define restrict __restrict

typedef enum memory_order {
    memory_order_relaxed, /*< No ordering */
    memory_order_consume,
    memory_order_acquire, /*< Synchronize my loads with others stores */
    memory_order_release, /*< Synchronize my stores with others loads */
    memory_order_acq_rel, /*< Synchronize my loads and stores with others loads and stores */
    memory_order_seq_cst
} memory_order;

typedef enum MOrder {
    RELAXED,
    CONSUME, 
    ACQUIRE, /*< No hoisting */
    RELEASE, /*< No sinking */
    ACQ_REL, SEQ_CST
} MOrder;

#define compilerMemBarrier() __atomic_signal_fence(memory_order_seq_cst)

//~ typedef __uint128_t u128;
//~ typedef __int128_t s128;
typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;

//~ typedef u128 uf128;
//~ typedef s128 sf128;
typedef u64 uf64;
typedef s64 sf64;
typedef u32 uf32;
typedef s32 sf32;
typedef uf32 uf16;
typedef sf32 sf16;
typedef uf16 uf8;
typedef sf16 sf8;

typedef uintptr_t uptr;
typedef intptr_t sptr;
typedef uptr ufptr;
typedef sptr sfptr;

#if 1
#define attrib(...) __attribute((__VA_ARGS__))
/*! The function function returns a value that should be used */
#define FA_WARN_UNUSED_RESULT attrib(warn_unused_result)
/*! The function function is should not be used */
#define FA_DEPRECATED attrib(deprecated)
/*! The function expects that the given argument numbers are not pointers to 0 */
#define FA_NONNULL(...) attrib(nonnull(__VA_ARGS__))
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
/*! The function returns a pointer p for which (uptr(p) % alignment) == offset. */
#define FA_ASSUME_ALIGNED(alignment, offset) attrib(assume_aligned(alignment, offset))
/*! The function returns a pointer that is not 0 */
#define FA_RETURNS_NONNULL attrib(returns_nonnull)
#else
/*! The function returns a pointer p for which (uptr(p) % alignment) == offset. */
#define FA_ASSUME_ALIGNED(alignment, offset) 
/*! The function returns a pointer that is not 0 */
#define FA_RETURNS_NONNULL 
#endif
/*! Call and return from the function before main. */
#define FA_CONSTRUCTOR attrib(constructor)
/*! The function should be inlined. */
#define FA_NO_INLINE attrib(noinline)
/*! The function should be inlined. */
#define FA_ALWAYS_INLINE attrib(always_inline)
/** Inline all calls to subroutines */
#define FA_FLATTEN attrib(flatten)
/*! The function does not return */
#define FA_NORETURN attrib(noreturn)
/*! The function has no sideeffects */
#define FA_PURE attrib(pure)
/*! The function has no sideeffects and only depends on its arguments */
#define FA_CONST attrib(const)
/*! The function returns a new pointer, which cannot be aliased by any other pointer.
   Can be used for malloc and calloc style functions, but not realloc. */
#define FA_MALLOC attrib(malloc)
/*! The function returns a field with a size given by the product of the argument numbers */
#define FA_ALLOC_SIZE(...) attrib(alloc_size(__VA_ARGS__))
/*! The function returns a pointer to a field that aligned to argument number alignmentNo */
#define FA_ALLOC_ALIGN(alignmentNo) attrib(alloc_align(alignmentNo))
/** The function must be emitted even when doing whole program optimization. */
#define FA_EXTERNALLY_VISIBLE attrib(externally_visible)
/** The function will be optimized with the following flags. */
#define FA_OPTIMIZE(flags) attrib(optimize(flags))
/** The function is likely called, and should be heavily optimized. */
#define FA_HOT attrib(hot)
//#define FA_HOT
/** Variables of this type are at least aligned to the given alignment. */
#define TA_ALIGNED(alignment) attrib(aligned(alignment))
/** Do not warn if variables of this type are declared by unused. */
#define TA_UNUSED attrib(unused)
/** The type is deprecated, the compiler will warn if used. */
#define TA_DEPRECATED attrib(deprecated)
/** The type may alias any other type, like chars. */
#define TA_MAY_ALIAS attrib(may_alias)
/** The type is a vector of a given byte length. */
#define TA_VECTOR_SIZE(length) attrib(vector_size(length))

#endif

#if onArm
#define VEC_SIZE (16)
#elif onX86
#define VEC_SIZE (32)
#else
#define VEC_SIZE (8)
#endif

#ifndef PAGE_SIZE
#if onSparc
#define PAGE_SIZE 8192
#else
#define PAGE_SIZE 4096
#endif
#endif

//#if onX86
//
//static inline u64 getTimeStamp() {
//    compilerMemBarrier();
//    u64 a;
//    u64 d;
//    asm volatile ("rdtscp" : "=a"(a), "=d"(d) : : "ecx");
//    compilerMemBarrier();
//    return a | (d << 32);
//}
//#else

static inline u64 getTimeStamp() {
    struct timeval wall_timer;
    compilerMemBarrier();
    gettimeofday(&wall_timer, 0);
    compilerMemBarrier();
    return (wall_timer.tv_sec * (u64) 1000000) + wall_timer.tv_usec;
}
//#endif

#if (BLYSK_ENABLE_SPEC || BLYSK_ENABLE_SLR || BLYSK_ENABLE_DATASPEC) && onX86 != 0
#define AtomicAbort(code)\
    compilerMemBarrier(); asm volatile ("xabort %0" : : "i"(code)); compilerMemBarrier();

/** Returns whether mid transaction */
static inline int AtomicTest() {
    asm volatile goto("xtest; jz %l[nonTrans]"::::nonTrans);
    return 1;
nonTrans:
    return 0;
}

/** Start a transaction.
 *  If the transaction fails goto label, and store the error code in error.
 *  Transactions memory operations complete atomically. */
#define AtomicBegin(label, error)\
    compilerMemBarrier(); asm volatile ("" : "=a"(error)); asm goto (".byte 0xc7,0xf8 ; .long %l1-.-4" : : "a"(error) : : label); compilerMemBarrier()
/** End a transaction.
 *  Transactions memory operations complete atomically. */
#define AtomicEnd \
    compilerMemBarrier(); asm volatile (".byte 0x0f,0x01,0xd5"); compilerMemBarrier()
#else // onX86
#define AtomicAbort(code) BI_UNREACHABLE

/** Returns whether mid transaction */
static inline int AtomicTest() {
    return 0;
}

/** Start a transaction.
 *  If the transaction fails goto label, and store the error code in error.
 *  Transactions memory operations complete atomically. */
#define AtomicBegin(label, error)\
    compilerMemBarrier(); goto error
/** End a transaction.
 *  Transactions memory operations complete atomically. */
#define AtomicEnd \
    BI_UNREACHABLE

#endif // ENABLE_SPEC != 0 && onX86

#define die() (compilerMemBarrier(), *((volatile char*) 0) = 0, BI_UNREACHABLE)

#define sassert(cond) ( (!expectTrue(cond)) ? die() : (void)0)

#ifndef NDEBUG
#define assert(cond) sassert(cond)
#define assume(cond) assert(cond)
#else
#define assert(cond) ((void)0)
//~ #define assume(cond) ((void)0)
#define assume(cond) (expectTrue(cond) ? (void) 0 : BI_UNREACHABLE)
#endif
