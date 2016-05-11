#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <blysk_config.h>
#include "utils.h"

#if onArm || onSparc
#define MALLOC_ALIGNMENT (8)
#elif onX86
#define MALLOC_ALIGNMENT (16)
#else
#define MALLOC_ALIGNMENT (sizeof(void*))
#endif



#if BLYSK_ENABLE_FAST_MALLOC > 1

#include "freestack.h"

/** The largest objects stored in the free stack */
#define FAST_ALLOC_MAX (640)
/** The granularity of the free stack objects */
#define FAST_ALLOC_DIV (64)
#if FAST_ALLOC_DIV != 64
#error "FAST_ALLOC_DIV must be 64"
#endif

typedef struct FSN_obj_t {
    uptr size;
    FSN* stack;
} FSN_obj;

extern __thread FSN_obj FSN_me[FAST_ALLOC_MAX / FAST_ALLOC_DIV];
extern GFS FSN_global[FAST_ALLOC_MAX / FAST_ALLOC_DIV];
extern TA_ALIGNED(PAGE_SIZE) unsigned char FSN_pages[PAGE_SIZE * (1ull << 15)];
extern unsigned char* FSN_pagePtr;
#ifndef NDEBUG
extern uptr totAllocs, totFrees;
#endif

static inline uintptr_t getAllocIndex(uintptr_t size) {
    assume(size != 0);
    return (size-1) / 64;
}

__attribute((warn_unused_result, malloc, returns_nonnull, assume_aligned(64), alloc_size(1)))
static inline void* alloc(uintptr_t size) {
    if(expectFalse(size > FAST_ALLOC_MAX)) {
        return malloc(size);
    }
#ifndef NDEBUG
    __atomic_fetch_add(&totAllocs, 1, ACQ_REL);
#endif
    uf16 index = getAllocIndex(size);
    assert(index < FAST_ALLOC_MAX / FAST_ALLOC_DIV);
    void* res = FSN_alloc(
            &FSN_me[index].size,
            &FSN_me[index].stack,
            &FSN_global[index].stack,
            &FSN_global[index].lock,
            &FSN_pagePtr,
            (index + 1) * 64);
    assert(sizeof(FSN_pages) + size + FSN_pages > (unsigned char*)res);
    return res;
}

__attribute((nonnull(1)))
static inline void dealloc(void* ptr, uintptr_t size) {
    if(expectFalse(size > FAST_ALLOC_MAX)) {
        free(ptr);
        return;
    }
#ifndef NDEBUG
    __atomic_fetch_add(&totFrees, 1, ACQ_REL);
#endif
    uf16 index = getAllocIndex(size);
    assert(index < FAST_ALLOC_MAX / FAST_ALLOC_DIV);
    FSN_dealloc(
            &FSN_me[index].size,
            &FSN_me[index].stack,
            &FSN_global[index].stack,
            &FSN_global[index].lock,
            ptr);
}

#elif BLYSK_ENABLE_FAST_MALLOC == 1

typedef struct FreeStackNode_t {
    struct FreeStackNode_t* next;
} FreeStackNode;

typedef struct {
    FreeStackNode* head;
    uintptr_t size;
} FreeStack;

/** The largest objects stored in the free stack */
#define FAST_ALLOC_MAX (640)
/** The granularity of the free stack objects */
#define FAST_ALLOC_DIV (64)
/** The most objects stored on any free stack */
#define FAST_ALLOC_CAP (16)

static inline uintptr_t getAllocIndex(uintptr_t size) {
    return (size + 7) / FAST_ALLOC_DIV;
}

static inline uintptr_t getAllocSize(uintptr_t index) {
    return index * FAST_ALLOC_DIV + (FAST_ALLOC_DIV - 8);
}

extern __thread FreeStack mypools[FAST_ALLOC_MAX / FAST_ALLOC_DIV + 1];

static inline void* _alloc(uintptr_t size) {
    uintptr_t index = getAllocIndex(size);
    if (expectFalse(index > FAST_ALLOC_MAX / FAST_ALLOC_DIV) || expectFalse(mypools[index].size == 0)) {
        return malloc(getAllocSize(index));
    }
    assume(index <= FAST_ALLOC_MAX / FAST_ALLOC_DIV);
    // Pop an object from the free stack
    FreeStackNode* r = mypools[index].head;
    mypools[index].size--;
    mypools[index].head = r->next;
    return r;
}

__attribute((malloc, returns_nonnull, assume_aligned(MALLOC_ALIGNMENT), alloc_size(1)))
static inline void* alloc(uintptr_t size) {
    void* r = _alloc(size);
    assume(0 == ((MALLOC_ALIGNMENT - 1) & (uintptr_t) r));
    assume(r != 0);
    return r;
}

__attribute((nonnull(1)))
static inline void dealloc(void* ptr, uintptr_t size) {
    assume(0 == ((MALLOC_ALIGNMENT - 1) & (uintptr_t) ptr));
    assume(ptr != 0);
    uintptr_t index = getAllocIndex(size);
    if (expectFalse(index > FAST_ALLOC_MAX / FAST_ALLOC_DIV) || expectFalse(mypools[index].size == FAST_ALLOC_CAP)) {
        free(ptr);
        return;
    }
    // Push an object to the free stack
    FreeStackNode* r = (FreeStackNode*) ptr;
    mypools[index].size++;
    r->next = mypools[index].head;
    mypools[index].head = r;
}
#else

static inline void* alloc(uintptr_t size) {
    return malloc(size);
}

static inline void dealloc(void* ptr, uintptr_t size) {
    free(ptr);
}
#endif
