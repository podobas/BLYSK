/* 
Copyright (c) 2013, Artur Podobas
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Artur Podobas nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Artur Podobas BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <pthread.h>
#include <stdbool.h>
#include <bomp.h>
#include <blysk_icv.h>
#include <blysk_thread.h>
#include <blysk_task.h>
#include <blysk_scheduler.h>
#include <blysk_public.h>
#include <slr_lock.h>
#include <spec.h>
#include <adapt.h>

// NOTE moved GOMP_task to blysk_task
#if BLYSK_ENABLE_SLR != 0
static TA_ALIGNED(64) TASLock critical_lock;

void GOMP_critical_start(void) {
    slrLock(&critical_lock, 2);
}

void GOMP_critical_end(void) {
    handleSpec();
    slrUnlock(&critical_lock);
}

void GOMP_critical_name_start (void **pptr) {
    TASLock* lock = (TASLock*) pptr;
    slrLock(lock, 2);
}

void GOMP_critical_name_end (void **pptr) {
    TASLock* lock = (TASLock*) pptr;
    slrUnlock(lock);
}
#else
static pthread_mutex_t critical_lock = PTHREAD_MUTEX_INITIALIZER;

void GOMP_critical_start(void) {
    handleSpec();
    pthread_mutex_lock(&critical_lock);
}

void GOMP_critical_end(void) {
    handleSpec(); 
    pthread_mutex_unlock(&critical_lock);
}

void GOMP_critical_name_start (void **pptr) {
    TASLock* lock = (TASLock*) pptr;
    tasLock(lock);
}

void GOMP_critical_name_end (void **pptr) {
    TASLock* lock = (TASLock*) pptr;
    tasUnlock(lock);
}
#endif

char BLYSK__single(void) {
    return blysk__ICV_single();
}

char GOMP_single_start(void) {
    handleSpec();
    return BLYSK__single();
}

// TODO unify the use of TAS, and pthread locks
void omp_init_lock(omp_lock_t *lock) {
//    pthread_mutex_init(lock, NULL);
    tasInit((TASLock*)lock);
}
void omp_set_lock(omp_lock_t *lock) {
//    pthread_mutex_lock(lock);
    GOMP_critical_name_start((void**)lock);
}
void omp_unset_lock(omp_lock_t *lock) {
//    pthread_mutex_unlock(lock);
    GOMP_critical_name_end((void**)lock);
}

void omp_destroy_lock(omp_lock_t *lock) {
    return;
}
int omp_test_lock(omp_lock_t *lock) {
    handleSpec();
//    return pthread_mutex_trylock(lock);
    return tasTryLock((TASLock*)lock);
}
