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
#include <blysk_scheduler_api.h>
#include <blysk_GQueue.h>


static unsigned int workers = 0;

static GQueue **Distributed_Queue;

void SCHEDULER_Thread_Exit(blysk_thread *__restrict thr) { }

size_t SCHEDULER_System_Init(blysk_icv *icv) {
    fprintf(stderr, "[Schedule] Work-Stealing enabled...\n");

    Distributed_Queue = (GQueue **) malloc(icv->n_Expected.count * sizeof (GQueue *));
    workers = icv->n_Expected.count;
    unsigned i;
    for (i = 0; i < icv->n_Expected.count; i++) {
        Distributed_Queue[i] = GQueue_New();
    }
    return 0;
}

void SCHEDULER_Thread_Init(blysk_thread *thr) {
}

void SCHEDULER_Exit(void) {
    for (unsigned i = 0; i != workers; i++) {
        free(Distributed_Queue[i]);
    }
    free(Distributed_Queue);
}

pthread_mutex_t LOCK1;

void SCHEDULER_Release(blysk_thread *thr, blysk_task *task) {
    GQueue_Enqueue_Front(Distributed_Queue[thr->r_ID], task);
}

blysk_task * SCHEDULER_Fetch(blysk_thread *thr) {
    blysk_task *new_task = NULL;
    unsigned int target = thr->r_ID;

    unsigned cnt;
    new_task = GQueue_Pop_Front(Distributed_Queue[target]);
    if (new_task != NULL) return new_task;

    for (cnt = 0; cnt < workers; cnt++) {
        target = (target + 1) % workers;
        //target = rand()%workers;
        new_task = GQueue_Pop_Back(Distributed_Queue[target]);
        if (new_task != NULL) return new_task;
    }

    return NULL;
}

blysk_task * SCHEDULER_Blocked_Fetch(blysk_thread *thr) {
    blysk_task *new_task = NULL;
    unsigned int target = thr->r_ID;

    unsigned cnt;
    new_task = GQueue_Pop_Front(Distributed_Queue[target]);
    if (new_task != NULL) return new_task;
    for (cnt = 0; cnt < workers; cnt++) {
        target = (target + 1) % workers;
        new_task = GQueue_Pop_Back(Distributed_Queue[target]);
        if (new_task != NULL) return new_task;
    }

    return NULL;
}


