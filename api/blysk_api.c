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
#include <unistd.h>
#include <sys/time.h>

#include <blysk_public.h>
#include <blysk_icv.h>

#include "utils.h"

#define NOT_IMPLEMENTED { fprintf(stderr,"Function %s not implemented.\n",__func__);abort();}

void omp_set_num_threads(int num_threads) {
    NOT_IMPLEMENTED;
}

int omp_get_num_threads(void) {
    return blysk__ICV_threads();
}

int omp_get_max_threads(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int omp_get_thread_num(void) {
    return blysk__THREAD_get_rid();
}

int omp_get_num_procs(void) {
    return omp_get_max_threads();
}

int omp_in_parallel(void) {
    return blysk__thread != 0;
//    NOT_IMPLEMENTED;
}

void omp_set_dynamic(int dynamic_threads) {
    /* Does nothing */
    if(dynamic_threads) {
        NOT_IMPLEMENTED;
    }
}

int omp_get_dynamic(void) {
    return 0; /* Not enabled */
}

int omp_get_cancellation(void) {
    return 0; /* Not enabled */
}

void omp_set_nested(int nested) {
    if(nested) {
        /* Not enabled */
        NOT_IMPLEMENTED;
    }
}

int omp_get_nested(void) {
    return 0; /*Not enabled */
}

void omp_set_schedule(omp_sched_t kind, int modifier) {
    NOT_IMPLEMENTED;
}

void omp_get_schedule(omp_sched_t *kind, int *modifier) {
    NOT_IMPLEMENTED;
}

int omp_get_thread_limit(void) {
    /* TODO return the minimum of the two:
     * 1. /proc/sys/kernel/threads-max
     * 2. /proc/sys/vm/max_map_count */
    NOT_IMPLEMENTED;
}

void omp_set_max_active_levels(int max_levels) {
    NOT_IMPLEMENTED;
}

int omp_get_max_active_levels(void) {
    return 1;
}

int omp_get_level(void) {
    NOT_IMPLEMENTED;
    // return blysk__thread->icv != 0;
}

int omp_get_ancestor_thread_num(void) {
    NOT_IMPLEMENTED;
}

int omp_get_team_size(void) {
    NOT_IMPLEMENTED;
}

int omp_get_active_level(void) {
    NOT_IMPLEMENTED;
    // return blysk__thread != 0;
}

int omp_in_final(void) {
    return 0;
}

omp_proc_bind_t omp_get_proc_bind(void) {
    if(blysk__thread == 0) {
        NOT_IMPLEMENTED;
    }
    return (omp_proc_bind_t) blysk__thread->icv->bind_threads;
}

void omp_set_default_device(int device_num) {
    NOT_IMPLEMENTED;
}

int omp_get_default_device(void) {
    NOT_IMPLEMENTED;
}

int omp_get_num_devices(void) {
//    NOT_IMPLEMENTED;
    return 0;
}

int omp_get_num_teams(void) {
    NOT_IMPLEMENTED;
}

int omp_get_team_num(void) {
    NOT_IMPLEMENTED;
}

double omp_get_wtime(void) {
    return getTimeStamp() / 1000000.0;
}

double omp_get_wtick(void) {
    return (1.0 / 1000000.0);
}

