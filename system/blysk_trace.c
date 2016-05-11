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

#include <blysk_trace.h>
#include <time.h>
#include <blysk_thread.h>


blysk__TRACE_context trace_ctx;
void (*program_f[256])(int, blysk__TRACE_record *, blysk__TRACE_record *);

unsigned long wall_time(void) {
    struct timeval wall_timer;
    gettimeofday(&wall_timer, NULL);
    return wall_timer.tv_sec * 1000000 + wall_timer.tv_usec;
}

void blysk__TRACE_init(int nt) {

#if defined(__HAS_PAPI)
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
        fprintf(stderr, "Failed to initialized PAPI..\n");
        abort();
    }

    if (PAPI_thread_init((long unsigned int (*)(void)) blysk__THREAD_get_rid) != PAPI_OK) {
        fprintf(stderr, "Failed to initialized PAPI thread init..\n");
        abort();
    }
#endif

    fprintf(stderr, "Started a tracing session...\n");
    int ir;
    trace_ctx.n_tracethreads = nt;
    trace_ctx.EventSet = (int *) malloc(sizeof (int) * trace_ctx.n_tracethreads);
    for (ir = 0; ir < trace_ctx.n_tracethreads; ir++) trace_ctx.EventSet[ir] = PAPI_NULL;
    trace_ctx.trace_data = (void **) malloc(sizeof (void**) * trace_ctx.n_tracethreads);
    trace_ctx.trace_end = (void **) malloc(sizeof (void**) * trace_ctx.n_tracethreads);
    for (ir = 0; ir < trace_ctx.n_tracethreads; ir++) {
        trace_ctx.trace_data[ir] = NULL;
        trace_ctx.trace_end[ir ] = NULL;
    }


    trace_ctx.trace_comm = (blysk__TRACE_comm *) malloc(TRACE_CHUNK);
    trace_ctx.trace_comm_end = trace_ctx.trace_comm; //(blysk__TRACE_comm *)  (trace_ctx.trace_comm + TRACE_CHUNK);


    trace_ctx.ThreadModifier = 1;

    /* Select program */
#if defined(__HAS_PAPI)


    if (getenv("OMP_TRACE_PROGRAM") == NULL) {
        trace_ctx.Program_Set[0] = PAPI_TOT_CYC;
        trace_ctx.Program_Set[1] = PAPI_TOT_INS;
        trace_ctx.Program_Set[2] = PAPI_RES_STL;
        trace_ctx.Program_Set[3] = PAPI_NULL;
        trace_ctx.Program = PERFORMANCE_PROGRAM;
        trace_ctx.ThreadModifier = 4;
    }
    else {
        char *program = getenv("OMP_TRACE_PROGRAM");
        {
            trace_ctx.Program_Set[0] = PAPI_TOT_CYC;
            trace_ctx.Program_Set[1] = PAPI_RES_STL;
            trace_ctx.Program_Set[2] = PAPI_TOT_INS;
            trace_ctx.Program_Set[3] = PAPI_NULL;
            trace_ctx.Program = PERFORMANCE_PROGRAM;
            trace_ctx.ThreadModifier = 4;
        }

        if (strcmp(program, "l1cache") == 0) {
            trace_ctx.Program_Set[0] = PAPI_TOT_CYC;
            trace_ctx.Program_Set[1] = PAPI_RES_STL;
            trace_ctx.Program_Set[2] = PAPI_L1_DCA;
            trace_ctx.Program_Set[3] = PAPI_L1_DCM;
            trace_ctx.Program = L1_PROGRAM;
            trace_ctx.ThreadModifier = 3;
        }

        if (strcmp(program, "l2cache") == 0) {
            trace_ctx.Program_Set[0] = PAPI_TOT_CYC;
            trace_ctx.Program_Set[1] = PAPI_L2_DCA;
            trace_ctx.Program_Set[2] = PAPI_L2_DCM;
            trace_ctx.Program_Set[3] = PAPI_NULL;
            trace_ctx.Program = L2_PROGRAM;
            trace_ctx.ThreadModifier = 3;
        }

        if (strcmp(program, "l3cache") == 0) {
            trace_ctx.Program_Set[0] = PAPI_L3_TCA;
            trace_ctx.Program_Set[1] = PAPI_L3_TCM;
            trace_ctx.Program_Set[2] = PAPI_NULL;
            trace_ctx.Program = L3_PROGRAM;
            trace_ctx.ThreadModifier = 3;
        }
    }
#endif

    __sync_synchronize();

    trace_ctx.trace_time[0] = wall_time();

}

void blysk__TRACE_record_state(int thr, unsigned int state) {
    /* Check for unitialization */
    if (trace_ctx.trace_data[thr] == NULL) {
        trace_ctx.trace_data[thr] = (void *) malloc(TRACE_CHUNK);
        trace_ctx.trace_end[thr ] = (void *) trace_ctx.trace_data[thr];
#if defined(__HAS_PAPI)
        if (PAPI_create_eventset(&trace_ctx.EventSet[thr]) != PAPI_OK) {
            fprintf(stderr, "Failed creating Event set for Thread: %d\n", thr);
            abort();
        }
        int ci;
        for (ci = 0; ci < 4; ci++) {
            if (trace_ctx.Program_Set[ci] == PAPI_NULL) break;
            if (PAPI_add_event(trace_ctx.EventSet[thr], trace_ctx.Program_Set[ci]) != PAPI_OK) {
                fprintf(stderr, "Failed adding counter %d for Thread: %d\n", ci, thr);
                abort();
            }
        }

        if (PAPI_start(trace_ctx.EventSet[thr]) != PAPI_OK) {
            fprintf(stderr, "Failed starting event-set for Thread: %d\n", thr);
            abort();
        }
#endif

        /* First record must be cleared */
        blysk__TRACE_record * fr = (blysk__TRACE_record *) trace_ctx.trace_end[thr];
        fr->t_start = wall_time();
        fr->t_state = state;
        PAPI_reset(trace_ctx.EventSet[thr]);
        return;
    }

    blysk__TRACE_record * fr = (blysk__TRACE_record *) trace_ctx.trace_end[thr];
    if (state == fr->t_state) return;
#if defined(__HAS_PAPI)
    if (PAPI_read(trace_ctx.EventSet[thr], fr->hw_cnt) != PAPI_OK) {
        fprintf(stderr, "Failed reading hardware counter for Thread: %d\n", thr);
        abort();
    }
#endif

    trace_ctx.trace_end[thr] += sizeof (blysk__TRACE_record); // - sizeof( long_long [4]));
    fr = (blysk__TRACE_record *) trace_ctx.trace_end[thr];
    if (trace_ctx.trace_end[thr] - trace_ctx.trace_data[thr] >= TRACE_CHUNK) {
        fprintf(stderr, "OOM\n");
        abort();
    }

    fr->t_start = wall_time();
    fr->t_state = state;
#if defined(__HAS_PAPI)
    PAPI_reset(trace_ctx.EventSet[thr]);
#endif
}

void internal__TRACE_initialize_files(void) {
    trace_ctx.fil[F_GENERIC] = fopen("parallel.prv", "w");


    /* Initialize the traces */
    program_f[PERFORMANCE_PROGRAM] = internal__TRACE_program_PERFORMANCE;
    program_f[L1_PROGRAM] = internal__TRACE_program_L1_PROGRAM;
    program_f[L2_PROGRAM] = internal__TRACE_program_L2_PROGRAM;
    program_f[L3_PROGRAM] = internal__TRACE_program_L3_PROGRAM;



    /* Initialize the parallel sections information */
    FILE *fp = fopen("parallel.pcf", "w");
    fprintf(fp, "STATES\n \
		  %d\tNONE\n \
		  %d\tTask Work\n \
		  %d\tThread Idle\n \
		  %d\tThread Blocked\n \
		  %d\tDependency Submit\n \
		  %d\tDependency Release \n \
		  %d\tCritical Section \n", STATE_NONE, STATE_TASK, STATE_IDLE, STATE_BLOCK, STATE_DEP_SUBMIT, STATE_DEP_RELEASE, STATE_CRITICAL);

    /* % */
    int iz, pr;
    for (iz = 0; iz < trace_ctx.ThreadModifier - 1; iz++) {
        for (pr = 0; pr < 101; pr++)
            fprintf(fp, "%d\t%d\%\n", pr + ((iz + 1)*100), pr);
    }


    fprintf(fp, "STATES_COLOR\n \
		  %d\t{0,0,0}\n \
		  %d\t{0,255,0}\n \
		  %d\t{50,50,50}\n \
		  %d\t{255,150,0}\n \
		  %d\t{0,0,255}\n \
		  %d\t{50,50,200}\n \
		  %d\t{255,0,0} \n", STATE_NONE, STATE_TASK, STATE_IDLE, STATE_BLOCK, STATE_DEP_SUBMIT, STATE_DEP_RELEASE, STATE_CRITICAL);


    /* Color */
    for (iz = 0; iz < trace_ctx.ThreadModifier - 1; iz++) {
        for (pr = 0; pr < 101; pr++)
            fprintf(fp, "%d\t{%d,%d,%d}\n", pr + ((iz + 1)*100), ((iz + 1) & 0x1)*(pr + 50), ((iz + 1) & 0x2)*(pr + 50), (((iz + 1) & 0x11) >> 1)*(pr + 50));
    }

    fclose(fp);



}

unsigned long norm_time(unsigned long t) {
    return t - trace_ctx.trace_time[0];
}

unsigned int internal__TRACE_state_dump(int n_cpu, blysk__TRACE_record *fr_v, blysk__TRACE_record *nx_v) {
    blysk__TRACE_record *fr = (blysk__TRACE_record *) fr_v;
    blysk__TRACE_record *nx = (blysk__TRACE_record *) nx_v;


    if (nx_v == NULL) /*first record*/
        fprintf(trace_ctx.fil[F_GENERIC], "1:%d:1:1:%d:%lu:%lu:%d\n", 1, trace_ctx.ThreadModifier * n_cpu + 1, 0, norm_time(fr->t_start), STATE_NONE);
    else
        fprintf(trace_ctx.fil[F_GENERIC], "1:%d:1:1:%d:%lu:%lu:%d\n", 1, trace_ctx.ThreadModifier * n_cpu + 1, norm_time(fr->t_start), norm_time(nx->t_start), fr->t_state);

#if defined(__HAS_PAPI)
    if (nx_v != NULL && fr->t_state == STATE_TASK) if (nx_v != NULL) program_f[trace_ctx.Program](n_cpu, fr, nx);
#endif

    return sizeof (blysk__TRACE_record); // - sizeof( long_long [4]);
}

pthread_mutex_t _xx;

void blysk__TRACE_comm_record(int from, unsigned long sstart, int to, unsigned long send) {
    pthread_mutex_lock(&_xx);
    //blysk__TRACE_comm *upd = __sync_fetch_and_add ( &trace_ctx.trace_comm_end , sizeof(blysk__TRACE_comm));
    blysk__TRACE_comm *upd = trace_ctx.trace_comm_end;
    trace_ctx.trace_comm_end++;
    pthread_mutex_unlock(&_xx);

    upd->sender = from;
    upd->sender_time = sstart;
    upd->receiver = to;
    upd->receiver_time = send;
}

void blysk__TRACE_done(void) {
    trace_ctx.trace_time[1] = wall_time();

    internal__TRACE_initialize_files();

    fprintf(trace_ctx.fil[F_GENERIC], "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
    fprintf(trace_ctx.fil[F_GENERIC], "%d(%d):", 1, trace_ctx.ThreadModifier * trace_ctx.n_tracethreads); /* Num nodes, num cpus per node */
    fprintf(trace_ctx.fil[F_GENERIC], "%d:1(%d:1)\n\n", 1, trace_ctx.ThreadModifier * trace_ctx.n_tracethreads); /* Num applications */


    while (trace_ctx.trace_comm < trace_ctx.trace_comm_end) {
        fprintf(trace_ctx.fil[F_GENERIC], "3:1:1:1:%d:%lu:%lu:", trace_ctx.ThreadModifier * trace_ctx.trace_comm->sender + 1,
                norm_time(trace_ctx.trace_comm->sender_time), norm_time(trace_ctx.trace_comm->sender_time));

        fprintf(trace_ctx.fil[F_GENERIC], "1:1:1:%d:%lu:%lu:", trace_ctx.ThreadModifier * trace_ctx.trace_comm->receiver + 1,
                norm_time(trace_ctx.trace_comm->receiver_time), norm_time(trace_ctx.trace_comm->receiver_time));
        fprintf(trace_ctx.fil[F_GENERIC], "%d:%d\n", 1, 1);


        trace_ctx.trace_comm++; //=sizeof(blysk__TRACE_comm);
    }

    int n_cpu;
    for (n_cpu = 0; n_cpu < trace_ctx.n_tracethreads; n_cpu++) {
        blysk__TRACE_record *nx = NULL, *fr = (blysk__TRACE_record *) trace_ctx.trace_data[n_cpu];
        if (trace_ctx.trace_data[n_cpu] == trace_ctx.trace_end[n_cpu]) continue;
        trace_ctx.trace_data[n_cpu] += internal__TRACE_state_dump(n_cpu, fr, nx);

        while (trace_ctx.trace_data[n_cpu] != trace_ctx.trace_end[n_cpu]) {
            nx = (blysk__TRACE_record *) trace_ctx.trace_data[n_cpu];
            trace_ctx.trace_data[n_cpu] += internal__TRACE_state_dump(n_cpu, fr, nx);
            fr = nx;
            if (trace_ctx.trace_data[n_cpu] == trace_ctx.trace_end[n_cpu]) break;
        }
    }

    program_f[trace_ctx.Program](-1, NULL, NULL);

    /* Close files */
    fclose(trace_ctx.fil[F_GENERIC]);
}
