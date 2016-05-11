#pragma once

#include <sys/mman.h>
#include <blysk_config.h>
#include <utils.h>

/** Stack intended for workstealing of tasks of type _T.
 *  empty represents no task.
 *   c is the capacity of the stack. */
template<class _T, uptr c = uptr(1) << 22, _T _empty = _T(0)>
struct TA_ALIGNED(PAGE_SIZE) StealStack {
    typedef _T T;
    static const constexpr T empty = _empty;
    uptr h; /*< Index of the head of the stack. */
    T e[c]; /*< Elements of the stack. */
};

template<class S>
void alloc(uptr n, S*__restrict &__restrict stack, uptr*__restrict&__restrict tails) {
    uptr size = sizeof (S) * n + n * (n * sizeof (uptr));
#if BLYSK_ENABLE_MMAP != 0
    if ((size & (PAGE_SIZE - 1)) != 0) {
        size += PAGE_SIZE - (size & (PAGE_SIZE - 1));
        assume((size & (PAGE_SIZE - 1)) == 0);
    }
    void* b = mmap(0, size, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS), 0, 0);
#else
    void* b = calloc(1, size);
#endif
    stack = (S*) b;
    tails = (uptr*) (stack + n);
}

template<class S>
void free(uptr n, S* stack, uptr*) {
    assume(n != 0);
    assume(stack != 0);
#if BLYSK_ENABLE_MMAP != 0
    uptr size = sizeof (S) * n + n * (n * sizeof (uptr));
    munmap(stack, size);
#else
    ::free(stack);
#endif
    assume(n > 0);
}

template<class T, class S>
void push(S& restrict s, T t) {
    __atomic_store_n(&s.e[s.h], t, MOrder::RELEASE);
    __atomic_store_n(&s.h, s.h + 1, MOrder::RELAXED);
}

template<class S>
typename S::T pop(S& restrict s) {
    // SWAP out the head
    typename S::T ret = S::empty;
    if (expectFalse(s.h == 0)) {
        assume(ret != (void*)(sptr)-1ll);
        return ret;
    }
    __atomic_store_n(&s.h, s.h - 1, MOrder::RELAXED);
    ret = __atomic_exchange_n(&s.e[s.h], ret, MOrder::ACQUIRE);
    if (expectTrue(ret != S::empty)) {
        assume(ret != (void*)(sptr)-1ll);
        return ret;
    }

    // The head was already removed, loop over the rest of the stack
    do {
        do {
            if (s.h == 0) {
                assume(ret != (void*)(sptr)-1ll);
                return ret;
            }
            __atomic_store_n(&s.h, s.h - 1, MOrder::RELAXED);
        } while (__atomic_load_n(&s.e[s.h], MOrder::RELAXED) == S::empty);
        ret = __atomic_exchange_n(&s.e[s.h], ret, MOrder::ACQUIRE);
    } while (expectFalse(ret == S::empty));
    assume(ret != (void*)(sptr)-1ll);
    return ret;
}

template<class S, class T>
typename S::T steal(S& restrict s, T& restrict t) {
    typename S::T ret = S::empty;
    uptr h = __atomic_load_n(&s.h, MOrder::RELAXED);
    do {
        while(1) {
            if (expectFalse(t >= h)) {
//                t = 0;
                return S::empty;
            }
            
            if(__atomic_load_n(&s.e[t], MOrder::RELAXED) != S::empty) {
                break;
            }
            t++;
        }
        ret = __atomic_exchange_n(&s.e[t], ret, MOrder::ACQUIRE);
        assume(ret != (void*)(sptr)-1ll);
    } while (ret == S::empty);
    return ret;
}

template<class S>
void trim(S& restrict s) {
    sptr h = __atomic_load_n(&s.h, MOrder::RELAXED);
    while(h > 0 &&
            __atomic_load_n(&s.e[h - 1], MOrder::RELAXED) == S::empty) {
        h--;
    }
    __atomic_store_n(&s.h, h, MOrder::RELAXED);
}



