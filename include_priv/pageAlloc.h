#pragma once

#include <sys/mman.h>
#include <utils.h>

#ifdef	__cplusplus
extern "C" {
#endif

    FA_ALLOC_SIZE(1) FA_MALLOC FA_RETURNS_NONNULL FA_WARN_UNUSED_RESULT FA_ASSUME_ALIGNED(PAGE_SIZE, 0)
    static inline void* pageAlloc(uptr* __restrict size) {
        if ((*size & (PAGE_SIZE - 1)) != 0) {
            *size += PAGE_SIZE - (*size & (PAGE_SIZE - 1));
            assert((*size & (PAGE_SIZE - 1)) == 0);
        }
        return mmap(0, *size, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS), 0, 0);
    }


#ifdef	__cplusplus
}
#endif


