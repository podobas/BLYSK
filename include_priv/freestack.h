#pragma once

#include <utils.h>
#include <slr_lock.h>

/** The number of objects allocated at a time with the global allocator */
#define ALLOC_BLOCK (PAGE_SIZE / 64)

/** Free Stack Node, a node in a stack of freed objects */
typedef struct FSN_t {
    struct FSN_t* __restrict n;
    struct FSN_t* __restrict s;
} FSN;

/** Global free stack (keeps a lock) */
typedef TA_ALIGNED(64) struct GFS_t {
    TASLock lock;
    FSN* __restrict stack;
    char PADDING[64 - sizeof(TASLock) - sizeof(void*)];
} GFS;

static inline uptr FSN_getSize(FSN* n) {
    uptr r = 0;
    while (n != 0) {
        n = n->n;
        r++;
    }
    return r;
}

__attribute((nonnull(1, 2, 3)))
static inline void FSN_push(
        uptr*__restrict psize,
        FSN*__restrict*__restrict mfs,
        FSN*__restrict ptr) {
    assert(ptr != 0);
    assert(psize != 0);
    assert(mfs != 0);
    *psize = *psize + 1;
    ptr->n = *mfs;
    *mfs = ptr;
}

__attribute((nonnull(1, 2)))
static inline FSN* FSN_pop(
        uptr*__restrict psize,
        FSN*__restrict*__restrict mfs) {
    assert(psize != 0);
    assert(mfs != 0);
    *psize = *psize - 1;
    FSN*__restrict r = *mfs;
    *mfs = r->n;
    assert(r != 0);
    return r;
}

__attribute((nonnull(1, 2)))
static inline FSN* FSN_popGlobal(
        FSN* __restrict * __restrict hp,
        TASLock*__restrict lock) {
    assert(hp != 0);
    if (*hp == 0) {
        return 0;
    }
    tasLock(lock); // Lock();
    FSN* h = *hp;
    if (h == 0) {
        tasUnlock(lock); // Unlock();
        return 0;
    }
    assert(h->n != 0);
    assert(h->s != 0);
    FSN* r = h;
    FSN* l = r->s;
    *hp = l->n;
    tasUnlock(lock); // Unlock();
    l->n = 0;
    //~ assert(getSize(r) == ALLOC_BLOCK);
    return r;
}

__attribute((nonnull(1, 2, 3, 4)))
static inline void FSN_pushGlobal(
        FSN* __restrict * __restrict hp,
        TASLock*__restrict lock,
        FSN* __restrict n,
        FSN* __restrict s) {
    assert(hp != 0);
    assert(n != 0);
    assert(s != 0);
    //~ #ifndef NDEBUG
    //~ s->n = 0;
    //~ sassert(getSize(n) == ALLOC_BLOCK);
    //~ #endif
    tasLock(lock); // Lock();
    FSN* h = *hp;
    s->n = h;
    *hp = n;
    tasUnlock(lock); // Unlock();
}

/** Allocate by carving up a new page */
__attribute((nonnull(1, 2), warn_unused_result, malloc, assume_aligned(64), returns_nonnull, flatten, always_inline))
static inline void* FSN_carvePage(
        FSN*__restrict*__restrict mfs,
        unsigned char* *__restrict page,
        uf16 k) {
    FSN* n = (FSN*) __atomic_fetch_add(page, k * ALLOC_BLOCK, ACQ_REL);
    assert(n != 0);
    FSN* d = 0;
    uf16 i = 0;
    do {
        n->n = d;
        d = n;
        n = (FSN*) (k + (uptr)d);
    } while (++i != ALLOC_BLOCK - 1);
    *mfs = d;
    assert(n != 0);
    return n;
}

__attribute((nonnull(1, 2, 3, 4, 5), warn_unused_result, malloc, assume_aligned(64, 0), returns_nonnull, flatten, always_inline))
static inline void* FSN_alloc(
        uptr* __restrict size,
        FSN* __restrict * __restrict mfs,
        FSN* __restrict * __restrict gfs,
        TASLock*__restrict lock,
        unsigned char * * __restrict page,
        uf16 k) {
    assert(size != 0);
    assert(mfs != 0);
    assert(gfs != 0);
    assert(page != 0);
    assert((k & 63) == 0);
    if (expectTrue(*size != 0)) { // Reuse memory I deallocated, if possible
        return FSN_pop(size, mfs);
    }
    assert(*mfs == 0);
    *size = ALLOC_BLOCK - 1;
    FSN* r;
    if (expectTrue(0 != (r = FSN_popGlobal(gfs, lock)))) { // Reuse memory someone deallocated, if possible
        *mfs = r->n;
        //~ assert(*size == getSize(*mfs));
        return (void*) r;
    }
    return FSN_carvePage(mfs, page, k); // Otherwise, allocate new memory
}

__attribute((nonnull(1, 2, 3, 4, 5), flatten, always_inline))
static inline void FSN_deallocImp(
        uptr* __restrict psize,
        FSN*__restrict*__restrict mfs,
        FSN*__restrict*__restrict gfs,
        TASLock*__restrict lock,
        FSN* __restrict ptr) {
    assert(ptr != 0);
    assert((63 & (uptr)ptr) == 0);
    assert(psize != 0);
    assert(mfs != 0);
    assert(gfs != 0);
    uptr size = *psize;
    FSN_push(psize, mfs, ptr); // Deallocate to thread local stack
    if (expectTrue(size != ALLOC_BLOCK * 2 - 1)) {
        return; // Just return if thread local stack is small
    }
    // Otherwise, pop objects and put them on the global stack
    *psize = ALLOC_BLOCK;
    FSN* s = ptr;
    uf8 i = 0;
    do {
        s = s->n;
    } while (++i != ALLOC_BLOCK - 1);
    ptr->s = s;
    *mfs = s->n;
    //~ assert(*psize == getSize(*mfs));
    FSN_pushGlobal(gfs, lock, ptr, s);
}

__attribute((nonnull(1, 2, 3, 4, 5), flatten, always_inline))
static inline void FSN_dealloc(
        uptr* __restrict psize,
        FSN*__restrict*__restrict mfs,
        FSN*__restrict*__restrict gfs,
        TASLock*__restrict lock,
        void* ptr) {
    FSN_deallocImp(psize, mfs, gfs, lock, (FSN*) ptr);
}
