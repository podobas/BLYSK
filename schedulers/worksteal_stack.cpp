#include <blysk_scheduler_api.h>
#include <StealStack.hpp>

#if onX86
template<class T>
static inline void rdrand(T*__restrict t) {
    static_assert(sizeof (T) == 2 || sizeof (T) == 4 || sizeof (T) == 8, "Can only rdrand 16, 32, or 64 bit values");
    //        *t = getTimeStamp();
    asm volatile("1: rdrand %0; jnc 1b" : "=r"(*t));
}
#endif

/** The pseudo random number generator LCG.
 *  Fast, full period, 1 unsigned integer state. Useful if you need a pattern
 *  extrapolation of a single integer, rather than random data. Often used to
 *  seed RNGs with larger states. */
namespace lcg {

template<class T> T randConf(T*__restrict t, T aOff = T(0), T cOff = T(0)) {
    return *t = (*t) * (4899146724695426045ull + aOff * 4) + 215086656626371ull + T(cOff * 2);
}
}

template<class T, uf8 bits = sizeof (T) * 8 > struct RollRight {

    static inline constexpr T v(T v) {
        return RollRight<T, (bits >> 1)>::v(v) | (RollRight<T, (bits >> 1)>::v(v) >> (bits >> 1));
    }
};

template<class T> struct RollRight<T, 0> {

    static constexpr T v(T v) {
        return v;
    }
};

template<class T, uf8 bits = sizeof (T) * 8 > constexpr T rollRight(T v) {
    return RollRight<T, bits>::v(v);
}


static unsigned nThreads;
static unsigned nThreadMask;
//static unsigned ctr;
//static __thread unsigned tid;
//static __thread unsigned prng;
//static __thread unsigned victim;

struct TL {
    u32 prng, victim, tail;
};

static StealStack<blysk_task*> * __restrict stacks;
static uptr* __restrict tails;
//static blysk_thread* threads[128];

FA_FLATTEN static inline void pickVictim(TL*__restrict tl, u32 tid) {
    assume(nThreads != 0);
    tl->tail = 0;
    if (nThreads == 1) {
        return;
    }
    do {
        tl->victim = lcg::randConf(&tl->prng, u32(0), tid) & nThreadMask;
    } while (tl->victim == tid || tl->victim >= nThreads);
}

static blysk_icv* _icv;

/** Called when a team is created. The *icv structure contains team info */
size_t SCHEDULER_System_Init(blysk_icv *__restrict icv) {
    _icv = icv;
    nThreads = icv->n_Threads;
    nThreadMask = rollRight(nThreads - 1);
    alloc(nThreads, stacks, tails);
    icv->opt_nowait = 0;
    icv->tlSize = sizeof(TL);
    return 0;
}

void SCHEDULER_Thread_Init(blysk_thread*__restrict thr) {
    u32 tid = thr->r_ID;
//    __atomic_store_n(threads + tid, thr, MOrder::RELAXED);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//    TL* tl = (TL*)&thr->metadata;
    TL* tl = (TL*)(thr + 1);
#pragma GCC diagnostic pop
    sassert(tid < nThreads);
    tl->prng = (tid + 1) % nThreads;
    tl->tail = 0;
    pickVictim(tl, tid);
    *((volatile uptr*)&stacks[tid].h) = 1; // Be the first to touch my stack
    *((volatile uptr*)&stacks[tid].h) = 0;
}

void SCHEDULER_Exit() {
    free(nThreads, stacks, tails);
}

/** Called when a thread wants to release a task */
void SCHEDULER_Release(blysk_thread*__restrict thr, blysk_task*__restrict task) {
    u32 tid = thr->r_ID;
    assume(tid < nThreads);
#if BLYSK_ENABLE_DATASPEC != 0
    task->self = &stacks[tid].e[stacks[tid].h];
#endif
#if BLYSK_ENABLE_FANOUT_CONTROL != 0
    if(expectFalse(stacks[tid].h >= 512)) {
        trim(stacks[tid]);
        thr->fanout = stacks[tid].h < 512;
    }
#endif
    assume(task != (void*)(sptr)-1ll);
    push(stacks[tid], task);
    
}

FA_FLATTEN static inline blysk_task* myfetch(blysk_thread*__restrict thr) {
    u32 tid = thr->r_ID;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    TL* tl = (TL*)(thr + 1);
#if BLYSK_ENABLE_FANOUT_CONTROL != 0
    thr->fanout = 1;
#endif
#pragma GCC diagnostic pop
    // See if you have a task
    blysk_task* t = pop(stacks[tid]);
    if (expectTrue(t != 0)) {
        assume(t != (void*)(sptr)-1ll);
        return t;
    }
    // Otherwise steal from your victim
    t = steal(stacks[tl->victim], tl->tail);
    if (t != 0) {
        assume(t != (void*)(sptr)-1ll);
        return t;
    }

#if BLYSK_ENABLE_FANOUT_CONTROL != 0
    __atomic_store_n(&_icv->booted_threads[tl->victim]->fanout, 1, MOrder::RELAXED);
#endif
    // Otherwise pick a new victim to steal from
    pickVictim(tl, tid);
    return 0;
}

/** Called when a thread has nothing to do */
blysk_task * SCHEDULER_Fetch(blysk_thread*__restrict t) {
    return myfetch(t);
}

/** Called when a thread is blocked and is looking for work */
extern "C" blysk_task* SCHEDULER_Blocked_Fetch(blysk_thread*__restrict t) __attribute__ ((alias ("SCHEDULER_Fetch")));

#include <stdio.h>
void SCHEDULER_Thread_Exit(blysk_thread *__restrict thr) {
    
}
