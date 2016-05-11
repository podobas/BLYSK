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

#include "blysk_trace.h"
#include <time.h>


int rgb[256][3] = {{0,0,0},{0,0,2},{0,0,4},{0,0,6},{0,0,8},{0,0,10},{0,0,12},{0,0,14},{0,0,16},{0,0,18},{0,0,20},{0,0,22},{0,0,24},{0,0,26},{0,0,28},{0,0,30},{0,0,32},{0,0,34},{0,0,36},{0,0,38},{0,0,40},{0,0,42},{0,0,44},{0,0,46},{0,0,48},{0,0,50},{0,0,52},{0,0,54},{0,0,56},{0,0,58},{0,0,60},{0,0,62},{0,0,64},{8,0,62},{16,0,60},{24,0,58},{32,0,56},{40,0,54},{48,51,52},{56,0,50},{64,0,48},{72,-2105519760,46},{80,0,44},{88,-848172880,42},{96,0,40},{104,47,38},{112,51,36},{120,16,34},{128,0,32},{136,909012736,30},{144,51,28},{152,111194217,26},{160,0,24},{168,-2105519328,22},{176,51,20},{184,111214452,18},{192,0,16},{200,-848172792,14},{208,51,12},{216,111213785,10},{224,51,8},{232,-2105519680,6},{240,32757,4},{248,113382016,2},{255,0,3892376},{255,8,32767},{255,16,6},{255,24,32767},{255,32,138284},{255,40,0},{255,48,0},{255,56,0},{255,64,4096},{255,72,0},{255,80,474657789},{255,88,0},{255,96,1372813738},{255,104,0},{255,112,0},{255,120,0},{255,128,4195056},{255,136,0},{255,144,113377280},{255,152,0},{255,160,111182579},{255,168,51},{255,176,0},{255,184,0},{255,192,17},{255,200,0},{255,208,10},{255,216,32757},{255,224,0},{255,232,0},{255,240,-848172816},{255,248,32767},{255,255,0},{255,255,4},{255,255,8},{255,255,12},{255,255,16},{255,255,20},{255,255,24},{255,255,28},{255,255,32},{255,255,36},{255,255,40},{255,255,44},{255,255,48},{255,255,52},{255,255,56},{255,255,60},{255,255,64},{255,255,68},{255,255,72},{255,255,76},{255,255,80},{255,255,84},{255,255,88},{255,255,92},{255,255,96},{255,255,100},{255,255,104},{255,255,108},{255,255,112},{255,255,116},{255,255,120},{255,255,124},{255,255,64},{255,255,68},{255,255,72},{255,255,76},{255,255,80},{255,255,84},{255,255,88},{255,255,92},{255,255,96},{255,255,100},{255,255,104},{255,255,108},{255,255,112},{255,255,116},{255,255,120},{255,255,124},{255,255,128},{255,255,132},{255,255,136},{255,255,140},{255,255,144},{255,255,148},{255,255,152},{255,255,156},{255,255,160},{255,255,164},{255,255,168},{255,255,172},{255,255,176},{255,255,180},{255,255,184},{255,255,188},{255,255,128},{255,255,132},{255,255,136},{255,255,140},{255,255,144},{255,255,148},{255,255,152},{255,255,156},{255,255,160},{255,255,164},{255,255,168},{255,255,172},{255,255,176},{255,255,180},{255,255,184},{255,255,188},{255,255,192},{255,255,196},{255,255,200},{255,255,204},{255,255,208},{255,255,212},{255,255,216},{255,255,220},{255,255,224},{255,255,228},{255,255,232},{255,255,236},{255,255,240},{255,255,244},{255,255,248},{255,255,252},{255,255,192},{255,255,193},{255,255,194},{255,255,195},{255,255,196},{255,255,197},{255,255,198},{255,255,199},{255,255,200},{255,255,201},{255,255,202},{255,255,203},{255,255,204},{255,255,205},{255,255,206},{255,255,207},{255,255,208},{255,255,209},{255,255,210},{255,255,211},{255,255,212},{255,255,213},{255,255,214},{255,255,215},{255,255,216},{255,255,217},{255,255,218},{255,255,219},{255,255,220},{255,255,221},{255,255,222},{255,255,223},{255,255,224},{255,255,225},{255,255,226},{255,255,227},{255,255,228},{255,255,229},{255,255,230},{255,255,231},{255,255,232},{255,255,233},{255,255,234},{255,255,235},{255,255,236},{255,255,237},{255,255,238},{255,255,239},{255,255,240},{255,255,241},{255,255,242},{255,255,243},{255,255,244},{255,255,245},{255,255,246},{255,255,247},{255,255,248},{255,255,249},{255,255,250},{255,255,251},{255,255,252},{255,255,253},{255,255,254},{255,255,255}};

extern blysk__TRACE_context trace_ctx;

void internal__TRACE_program_PERFORMANCE (int n_cpu, blysk__TRACE_record *fr_v , blysk__TRACE_record *nx_v)
{
   blysk__TRACE_record *fr = (blysk__TRACE_record *) fr_v;
   blysk__TRACE_record *nx = (blysk__TRACE_record *) nx_v;

   static FILE *fp_ipc = NULL, *fp_res = NULL;

   /* Closing */
   if (n_cpu == -1)
	{
	    return;
	}

   if (fp_ipc == NULL && n_cpu != -1) {
	 fp_ipc = fopen("parallel_ipc.pcf","w");
	 fp_res = fopen("parallel_res_stall.pcf","w");


       fprintf(fp_ipc,"STATES\n");fprintf(fp_res,"STATES\n");
       int col;        
        fprintf(fp_ipc,"0\tNone\n");
        fprintf(fp_res,"0\tNone\n");
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_ipc,"%d\t%d\n",col,col);
	   fprintf(fp_res,"%d\t%d\n",col,col); }

      fprintf(fp_ipc,"\nSTATES_COLOR\n");fprintf(fp_res,"\nSTATES_COLOR\n");
 	  fprintf(fp_ipc,"0\t {0,0,0} \n"); 
 	  fprintf(fp_res,"0\t {0,0,0} \n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_ipc,"%d\t{%d,%d,%d}\n",col,rgb[50+2*col][0],rgb[50+2*col][1],rgb[50+2*col][2]);
	   fprintf(fp_res,"%d\t{%d,%d,%d}\n",col,rgb[50+2*col][0],rgb[50+2*col][1],rgb[50+2*col][2]); }
      
      fclose(fp_ipc);fclose(fp_res); 

	 fp_ipc = fopen("parallel_ipc.prv","w");
	 fp_res = fopen("parallel_res_stall.prv","w");


   fprintf(fp_ipc, "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
   fprintf(fp_ipc, "%d(%d):",1,trace_ctx.n_tracethreads);   fprintf(fp_ipc, "%d:1(%d:1)\n", 1 , trace_ctx.n_tracethreads);

   fprintf(fp_res, "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
   fprintf(fp_res, "%d(%d):",1,trace_ctx.n_tracethreads);   fprintf(fp_res, "%d:1(%d:1)\n", 1 , trace_ctx.n_tracethreads);

   }

   /* IPC */
   {
	float sub = (float) fr->hw_cnt[2] / (float) fr->hw_cnt[0];
	int res = sub*10;
        fprintf(fp_ipc, "1:%d:1:1:%d:%lu:%lu:%d\n",1,n_cpu+1, norm_time(fr->t_start) , norm_time(nx->t_start), res);
   }

   /* RES_STALL */
   {
	float sub =  ((float) fr->hw_cnt[1]) / ((float) fr->hw_cnt[0]);
	int res = sub*100;
        if (fr->t_state == STATE_TASK) fprintf(fp_res, "1:%d:1:1:%d:%lu:%lu:%d\n",1,n_cpu+1, norm_time(fr->t_start) , norm_time(nx->t_start), res);
   }

}


void internal__TRACE_program_L1_PROGRAM (int n_cpu, blysk__TRACE_record *fr_v , blysk__TRACE_record *nx_v)
{
   blysk__TRACE_record *fr = (blysk__TRACE_record *) fr_v;
   blysk__TRACE_record *nx = (blysk__TRACE_record *) nx_v;

   static FILE *fp_l1 = NULL, *fp_res = NULL;
   static double L1_ACC[3] = {0,0,0};

   /* Closing */
   if (n_cpu == -1)
	{
	    fprintf(stderr,"[Profiler] The application experienced a total of: %f %%L1 misses\n",L1_ACC[1] / L1_ACC[0]);
	    fprintf(stderr,"[Profiler] Resource stall experienced: %f cycles\n",L1_ACC[2]);
	    return;
	}

   if (fp_l1 == NULL) {
	 fp_l1 = fopen("parallel_l1.pcf","w");
	 fp_res = fopen("parallel_res_stall.pcf","w");



       fprintf(fp_l1,"STATES\n");fprintf(fp_res,"STATES\n");
       int col;
        fprintf(fp_l1,"0\tNone\n"); 
        fprintf(fp_res,"0\tNone\n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_l1,"%d\t%d\n",col,col);
	   fprintf(fp_res,"%d\t%d\n",col,col); }

      fprintf(fp_l1,"\nSTATES_COLOR\n");fprintf(fp_res,"\nSTATES_COLOR\n");
 	  fprintf(fp_l1,"0\t {0,0,0} \n"); 
 	  fprintf(fp_res,"0\t {0,0,0} \n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_l1,"%d\t{%d,%d,%d}\n",col,rgb[50+2*col][0],rgb[50+2*col][1],rgb[50+2*col][2]);
	   fprintf(fp_res,"%d\t{%d,%d,%d}\n",col,rgb[50+2*col][0],rgb[50+2*col][1],rgb[50+2*col][2]); }
      
      fclose(fp_l1);fclose(fp_res); 

	 fp_l1 = fopen("parallel_l1.prv","w");
	 fp_res = fopen("parallel_res_stall.prv","w");


   fprintf(fp_l1, "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
   fprintf(fp_l1, "%d(%d):",1,trace_ctx.n_tracethreads);   fprintf(fp_l1, "%d:1(%d:1)\n", 1 , trace_ctx.n_tracethreads);

   fprintf(fp_res, "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
   fprintf(fp_res, "%d(%d):",1,trace_ctx.n_tracethreads);   fprintf(fp_res, "%d:1(%d:1)\n", 1 , trace_ctx.n_tracethreads);

   }

   /* L1-Cache  */
   {
	float sub = (float) fr->hw_cnt[3] / (float) fr->hw_cnt[2];
	int res = sub*100;
	if (res < 0) fprintf(stderr,"WTF\n");    
       if (fr->hw_cnt[2] != 0)
	{
	   L1_ACC[0] += fr->hw_cnt[2];
	   L1_ACC[1] += fr->hw_cnt[3];
	}

        fprintf(fp_l1, "1:%d:1:1:%d:%lu:%lu:%d\n",1,n_cpu+1, norm_time(fr->t_start) , norm_time(nx->t_start), res);
	res+=100;   
        fprintf(trace_ctx.fil[F_GENERIC], "1:%d:1:1:%d:%lu:%lu:%d\n",1,(trace_ctx.ThreadModifier*n_cpu)+2, norm_time(fr->t_start) , norm_time(nx->t_start), res);

   }

   /* RES_STALL */
   {
	float sub =  ((float) fr->hw_cnt[1]) / ((float) fr->hw_cnt[0]);
	int res = sub*100;
	L1_ACC[2] += (double) fr->hw_cnt[1];
        fprintf(fp_res, "1:%d:1:1:%d:%lu:%lu:%d\n",1,n_cpu+1, norm_time(fr->t_start) , norm_time(nx->t_start), res);
	res+=200;
        fprintf(trace_ctx.fil[F_GENERIC], "1:%d:1:1:%d:%lu:%lu:%d\n",1,(trace_ctx.ThreadModifier*n_cpu)+3, norm_time(fr->t_start) , norm_time(nx->t_start), res);

   }
}


void internal__TRACE_program_L2_PROGRAM (int n_cpu, blysk__TRACE_record *fr_v , blysk__TRACE_record *nx_v)
{
   blysk__TRACE_record *fr = (blysk__TRACE_record *) fr_v;
   blysk__TRACE_record *nx = (blysk__TRACE_record *) nx_v;

   static FILE *fp_l2 = NULL;

   static double app_time = 0;
   static double app_accum = 0;

   static double L2_ACC[2] = {0,0};

   /* Closing */
   if (n_cpu == -1)
	{
//	    fprintf(stderr,"[Trace] Average L3 cache miss ratio for application was: %f\n",app_accum / app_time);
//	    fprintf(stderr,"%f vs %f\n",app_time , app_accum);
	    fprintf(stderr,"[Profiler] The application experienced a total of: %f %%L2 misses\n",L2_ACC[1] / L2_ACC[0]);
	    return;
	}

   if (fp_l2 == NULL) {
	fprintf(stderr,"Level-2 program activated\n");
	 fp_l2 = fopen("parallel_l2.pcf","w");


       fprintf(fp_l2,"STATES\n");
       int col;
	   fprintf(fp_l2,"0\tNONE \n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_l2,"%d\t%d\n",col,col); }


      fprintf(fp_l2,"\nSTATES_COLOR\n");
	fprintf(fp_l2,"0\t {0,0,0} \n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_l2,"%d\t{%d,%d,%d}\n",col,rgb[50+2*col][0],rgb[50+2*col][1],rgb[50+2*col][2]); }
      
      fclose(fp_l2);

	 fp_l2 = fopen("parallel_l2.prv","w");


   fprintf(fp_l2, "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
   fprintf(fp_l2, "%d(%d):",1,trace_ctx.n_tracethreads);   fprintf(fp_l2, "%d:1(%d:1)\n", 1 , trace_ctx.n_tracethreads);

   }
   /* l2-Cache  */

   {
	float sub = (float) fr->hw_cnt[2] / (float) fr->hw_cnt[1];
	if (fr->hw_cnt[1] != 0) app_accum += (double)sub * ((double)norm_time(nx->t_start) - (double)norm_time(fr->t_start));
	app_time += ((double)norm_time(nx->t_start) - (double)norm_time(fr->t_start));	

       if (fr->hw_cnt[1] != 0)
	{
	   L2_ACC[0] += fr->hw_cnt[1];
	   L2_ACC[1] += fr->hw_cnt[2];
	}

	int res = sub*100;
        fprintf(fp_l2, "1:%d:1:1:%d:%lu:%lu:%d\n",1,n_cpu+1,                          norm_time(fr->t_start) , norm_time(nx->t_start), res);
	res+=100;
        fprintf(trace_ctx.fil[F_GENERIC], "1:%d:1:1:%d:%lu:%lu:%d\n",1,(trace_ctx.ThreadModifier*n_cpu)+2, norm_time(fr->t_start) , norm_time(nx->t_start), res);


   }
}


void internal__TRACE_program_L3_PROGRAM (int n_cpu, blysk__TRACE_record *fr_v , blysk__TRACE_record *nx_v)
{
   blysk__TRACE_record *fr = (blysk__TRACE_record *) fr_v;
   blysk__TRACE_record *nx = (blysk__TRACE_record *) nx_v;

   static FILE *fp_l3 = NULL;


   static double L3_ACC[2] = {0,0};

   static double app_time = 0;
   static double app_accum = 0;

   /* Closing */
   if (n_cpu == -1)
	{
	   // fprintf(stderr,"[Trace] Average L3 cache miss ratio for application was: %f\n",app_accum / app_time);
	    //fprintf(stderr,"%f vs %f\n",app_time , app_accum);
	    fprintf(stderr,"[Profiler] The application experienced a total of: %f %%L3 misses\n",L3_ACC[1] / L3_ACC[0]);
	    return;
	}

   if (fp_l3 == NULL) {
	fprintf(stderr,"Level-3 program activated\n");
	 fp_l3 = fopen("parallel_l3.pcf","w");


       fprintf(fp_l3,"STATES\n");
       int col;

	   fprintf(fp_l3,"0\tNone\n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_l3,"%d\t%d\%\n",col,col); }

      fprintf(fp_l3,"\nSTATES_COLOR\n");
	   fprintf(fp_l3,"0\t {0,0,0} \n"); 
	for (col=1;col < 101 ; col++) {
	   fprintf(fp_l3,"%d\t{%d,%d,%d}\n",col,rgb[50+2*col][0],rgb[50+2*col][1],rgb[50+2*col][2]); }
      
      fclose(fp_l3);

	 fp_l3 = fopen("parallel_l3.prv","w");


   fprintf(fp_l3, "#Paraver (10/10/2010 at 09:44):%lu:", trace_ctx.trace_time[1] - trace_ctx.trace_time[0]);
   fprintf(fp_l3, "%d(%d):",1,trace_ctx.n_tracethreads);   fprintf(fp_l3, "%d:1(%d:1)\n", 1 , trace_ctx.n_tracethreads);

   }
   
   /* l3-Cache  */

   {
	float sub = (float) fr->hw_cnt[1] / (float) fr->hw_cnt[0];
        if (fr->hw_cnt[0] != 0)
	{
	   L3_ACC[0] += fr->hw_cnt[0];
	   L3_ACC[1] += fr->hw_cnt[1];
	}

	if (fr->hw_cnt[0] != 0) app_accum += (double)sub * ((double)norm_time(nx->t_start) - (double)norm_time(fr->t_start));
	app_time += ((double)norm_time(nx->t_start) - (double)norm_time(fr->t_start));	
	int res = sub*100;
        fprintf(fp_l3, "1:%d:1:1:%d:%lu:%lu:%d\n",1,n_cpu+1, norm_time(fr->t_start) , norm_time(nx->t_start), res);
	res+=100;
        fprintf(trace_ctx.fil[F_GENERIC], "1:%d:1:1:%d:%lu:%lu:%d\n",1,(trace_ctx.ThreadModifier*n_cpu)+2, norm_time(fr->t_start) , norm_time(nx->t_start), res);

   }
}
