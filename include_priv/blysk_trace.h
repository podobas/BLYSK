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

#if defined (__HAS_PAPI)
#include <papi.h>
#endif

#define TRACE_CHUNK (1 << 27)

#define STATE_NONE 0x0
#define STATE_TASK 0x1
#define STATE_IDLE 0x2
#define STATE_BLOCK 0x3
#define STATE_DEP_SUBMIT 0x4
#define STATE_DEP_RELEASE 0x5
#define STATE_CRITICAL 0x6


#define F_GENERIC 0x0
#define F_RESOURCE 0x1
#define F_IPC 0x2
#define F_CACHE_L1 0x3
#define F_CACHE_L2 0x4
#define F_CACHE_L3 0x5

#define PERFORMANCE_PROGRAM 0x1
#define L1_PROGRAM 0x2
#define L2_PROGRAM 0x3
#define L3_PROGRAM 0x4

#if defined(__TRACE)
#define BLYSK_TRACE(thr,state)  blysk__TRACE_record_state(thr,state);
#else
#define BLYSK_TRACE(thr,state) ((void)0)
#endif

/* 
   Profiling configurations

   Caches: Cycles, L1-data, L2-data, L3-data
   Stalls: Cycles, Resource-stall, L2-data, L3-data
   Instruction: Cycles, Instruction count, L2-data,L3-data */


typedef struct {
    unsigned long t_start;
    unsigned int t_state;
#if defined (__HAS_PAPI)
    long_long hw_cnt[4];
#endif
} blysk__TRACE_record;

typedef struct {
    int sender;
    unsigned long sender_time;
    int receiver;
    unsigned long receiver_time;
} blysk__TRACE_comm;



typedef struct {
    int Program_Set[4];
    int Program;

    int *EventSet;

    void **trace_data;
    void **trace_end;
    int n_tracethreads;
    blysk__TRACE_comm *trace_comm;
    blysk__TRACE_comm *trace_comm_end;

    unsigned long trace_time[2];

    unsigned int parallel_section;

    void *fil[6];
    int ThreadModifier;

} blysk__TRACE_context;


void blysk__TRACE_init(int nt);
void blysk__TRACE_done(void);
void blysk__TRACE_record_state(int thr, unsigned int state);
unsigned long norm_time(unsigned long t);
unsigned long wall_time(void);

void internal__TRACE_program_PERFORMANCE(int n_cpu, blysk__TRACE_record *fr_v, blysk__TRACE_record *nx_v);
void internal__TRACE_program_L1_PROGRAM(int n_cpu, blysk__TRACE_record *fr_v, blysk__TRACE_record *nx_v);
void internal__TRACE_program_L2_PROGRAM(int n_cpu, blysk__TRACE_record *fr_v, blysk__TRACE_record *nx_v);
void internal__TRACE_program_L3_PROGRAM(int n_cpu, blysk__TRACE_record *fr_v, blysk__TRACE_record *nx_v);


