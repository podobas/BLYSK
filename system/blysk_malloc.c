#include <blysk_icv.h>
#include <blysk_thread.h>
#include <adapt.h>
#include <blysk_dlsym.h>

#include "blysk_malloc.h"

#if BLYSK_ENABLE_FAST_MALLOC > 1

__thread FSN_obj FSN_me[FAST_ALLOC_MAX / FAST_ALLOC_DIV];
GFS FSN_global[FAST_ALLOC_MAX / FAST_ALLOC_DIV];
TA_ALIGNED(PAGE_SIZE) unsigned char FSN_pages[PAGE_SIZE * (1ull << 15)];
unsigned char* FSN_pagePtr;
#ifndef NDEBUG
uptr totAllocs, totFrees;
#endif

__attribute((constructor)) void initBlyskMalloc() {
    FSN_pagePtr = FSN_pages;
    assert(totFrees == 0);
    assert(totAllocs == 0);
}

#elif BLYSK_ENABLE_FAST_MALLOC == 1

__thread FreeStack mypools[FAST_ALLOC_MAX / FAST_ALLOC_DIV + 1];

#endif





__attribute((constructor))
void blysk__ADAPTATION_new() {
#if BLYSK_ENABLE_ADAPT != 0
    blysk_icv *icv = &bpc;


    void *dll_handle; // TODO remember handle so we can close it nicely?
    char *designated_adaptation = getenv("OMP_TASK_ADAPTATION");
    if (designated_adaptation == NULL || *designated_adaptation == 0) {
        fprintf(stderr, "\tOMP_TASK_ADAPTION not set or not found. Will not use adaptation.\n");
        icv->adaptation.adapt_online = 0;
        return;
    }

    dll_handle = dlopen(designated_adaptation, RTLD_LAZY);

    if (!dll_handle) {
        fprintf(stderr, "\t'%s' not found. Set LD_LIBRARY_PATH to point to the location of the libraries...\n", designated_adaptation);
        abort();
    }

    icv->adaptation.adapt_main = (void*(*)(void*)) mydlsym(dll_handle, "ADAPT_Main", designated_adaptation);
    icv->adaptation.adapt_kill = mydlsym(dll_handle, "ADAPT_Kill", designated_adaptation);
    pthread_create(&icv->adaptation.h_ID, NULL, icv->adaptation.adapt_main, icv);
//    fprintf(stderr, "Started %s\n", designated_adaptation);
    icv->adaptation.adapt_online = 1;
#endif
}

__attribute((destructor, used))
void blysk__ADAPTATION_clean() {
#if BLYSK_ENABLE_ADAPT != 0
    blysk_icv *icv = (blysk_icv *) &bpc;
    if (icv->adaptation.adapt_online == 0) return;
    icv->adaptation.adapt_kill();
    pthread_join(icv->adaptation.h_ID, NULL);
#endif
}
