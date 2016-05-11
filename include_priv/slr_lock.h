#pragma once

#include <stdbool.h>
#include <blysk_public.h>
#include "utils.h"

static inline bool tasIsLocked(TASLock* l) {
    return __atomic_load_n(&l->l, memory_order_relaxed) != 0;
}

static inline void tasInit(TASLock* l) {
    __atomic_store_n(&l->l, 0, memory_order_release);
}

static inline void tasUnlock(TASLock* l) {
    tasInit(l);
}

static inline unsigned tasTryLock(TASLock* l) {
   return __atomic_exchange_n(&l->l, 1u, memory_order_acquire);
}

static inline void tasLock(TASLock* l) {
    while (tasTryLock(l) != 0);
}

#if onArm || onX86
static inline unsigned fetchAndAddCounter(Counter*__restrict c, unsigned val, MOrder order) {
    return __atomic_fetch_add(&c->count, val, order);
}
#else
static inline unsigned fetchAndAddCounter(Counter*__restrict c, unsigned val, MOrder order) {
    assert(order != SEQ_CST);
    tasLock(&c->lock);
    unsigned r = c->count;
    c->count += val;
    tasUnlock(&c->lock);
    return r;
}
#endif



#if BLYSK_ENABLE_SPEC != 0 || BLYSK_ENABLE_SLR

static inline bool slrLock(TASLock* l, sf16 retries) {
    u32 error;
start:
    AtomicBegin(fallback, error);
    return true;
fallback:
    retries--;
    if (retries >= 0) {
        goto start;
    }
    else {
        tasLock(l);
        return false;
    }
}

static inline void slrUnlock(TASLock* l) {
    if (AtomicTest()) {
        if (tasIsLocked(l)) {
            AtomicAbort(1);
        }
        else {
            AtomicEnd;
        }
    }
    else {
        tasUnlock(l);
    }
}

static inline bool slrLocklock2(TASLock* l, sf16 retries) {
    u32 error;
start:
    AtomicBegin(fallback, error);
    return true;
fallback:
    retries--;
    if (retries < 0) {
        tasLock(l);
        return false;
    }
    if (retries == 0) {
        while (tasIsLocked(l));
    }
    goto start;
}

#endif
