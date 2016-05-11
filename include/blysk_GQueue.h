#pragma once

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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <blysk_public.h>

typedef struct {
    pthread_mutex_t Glock;
    blysk_task *HTail[2];
    unsigned int __elements;
} GQueue;

#ifdef	__cplusplus
extern "C" {
#endif

static inline GQueue * GQueue_New(void);

static inline GQueue* GQueue_Init(GQueue* __restrict q) {
    q->HTail[0] = NULL;
    q->HTail[1] = NULL;
    q->__elements = 0;
    pthread_mutex_init(&q->Glock, NULL);
    __sync_synchronize();
    return q;
}

static inline GQueue * GQueue_New(void) {
    GQueue *q = (GQueue *) malloc(sizeof (GQueue));
    if (q == NULL) {
        fprintf(stderr, "Failed allocating queue\n");
        abort();
    }
    return GQueue_Init(q);
}

static inline void GQueue_Enqueue_Front(GQueue *q, blysk_task *_tsk) {
    pthread_mutex_lock(&q->Glock);
    if (q->HTail[0] == NULL) {
        q->HTail[0] = _tsk;
        q->HTail[1] = q->HTail[0];
        //	   __sync_synchronize(); 
        q->__elements++;
        //	   __sync_synchronize(); 
        pthread_mutex_unlock(&q->Glock);
        return;
    }
    _tsk->FPN[1] = q->HTail[0];
    _tsk->FPN[0] = NULL;
    q->HTail[0]->FPN[0] = _tsk;
    q->HTail[0] = _tsk;
    //        __sync_synchronize(); 
    q->__elements++;
    //       __sync_synchronize();
    pthread_mutex_unlock(&q->Glock);
}

static inline void GQueue_Enqueue_Back(GQueue *q, blysk_task *_tsk) {
    pthread_mutex_lock(&q->Glock);
    if (q->HTail[0] == NULL) {
        q->HTail[0] = _tsk;
        q->HTail[1] = q->HTail[0];
        //	   __sync_synchronize(); 
        q->__elements++;
        //           __sync_synchronize(); 
        pthread_mutex_unlock(&q->Glock);
        return;
    }
    _tsk->FPN[0] = q->HTail[1];
    q->HTail[1]->FPN[1] = _tsk;
    q->HTail[1] = _tsk;
    //        __sync_synchronize(); 
    q->__elements++;
    //           __sync_synchronize(); 
    pthread_mutex_unlock(&q->Glock);
}

static inline blysk_task *GQueue_Pop_Front(GQueue *q) {

    //    if (q->__elements == 0) return NULL;
    pthread_mutex_lock(&q->Glock);

    //    __sync_synchronize();

    if (/*q->__elements == 0 || */q->HTail[0] == NULL) {
        pthread_mutex_unlock(&q->Glock);
        return NULL;
    }

    if (q->HTail[0] == q->HTail[1]) {
        blysk_task *tmp = q->HTail[0];
        q->HTail[0] = NULL;
        q->HTail[1] = NULL;
        q->__elements--;
        if (q->__elements != 0) {
            fprintf(stderr, "Elements not %d\n", q->__elements);
            abort();
        }
        //	    __sync_synchronize(); 
        pthread_mutex_unlock(&q->Glock);
        return tmp;
    }

    blysk_task *tmp = q->HTail[0];
    q->HTail[0] = tmp->FPN[1];
    q->HTail[0]->FPN[0] = NULL;
    q->__elements--;
    //    __sync_synchronize(); 
    pthread_mutex_unlock(&q->Glock);
    return tmp;
}

static inline blysk_task *GQueue_Pop_Back(GQueue *q) {
    //   if (q->__elements == 0) return NULL;
    pthread_mutex_lock(&q->Glock);
    if (/*q->__elements == 0 || */q->HTail[0] == NULL) {
        pthread_mutex_unlock(&q->Glock);
        return NULL;
    }
    if (q->HTail[0] == q->HTail[1]) {
        void *tmp = q->HTail[0];
        q->HTail[0] = NULL;
        q->HTail[1] = NULL;
        q->__elements--;
        //	    __sync_synchronize(); 
        pthread_mutex_unlock(&q->Glock);
        return tmp;
    }

    blysk_task *tmp = q->HTail[1];

    q->HTail[1] = q->HTail[1]->FPN[0];
    q->HTail[1]->FPN[1] = NULL;
    q->__elements--;
    //    __sync_synchronize(); 
    pthread_mutex_unlock(&q->Glock);
    return tmp;
}

#ifdef __cplusplus 
}
#endif
