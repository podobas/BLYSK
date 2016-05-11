/* 
Copyright (c) 2016, Artur Podobas
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
#include <string.h>
#include <pthread.h>

#include <blysk_public.h>

#include <blysk_task.h>
#include <blysk_dep.h>
#include <blysk_trace.h>
#include <blysk_memory.h>
#include <blysk_malloc.h>
#include <circular_queue.h>
#include <utils.h>
#include <spec.h>
#include <slr_lock.h>
#include <blysk_transfer.h>
#include "blysk_transfer_imp.h"
#include "blysk_dep_imp.h"

/*
void
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	   long arg_size, long arg_align, bool if_clause, unsigned flags,
	   void **depend, int priority)
*/

typedef struct {
  long start,end,step;
  void (*fn)(void *);
  unsigned long arg_size;
  void *data;
  long arg_align;
  char *args;
  void (*cpyfn)(void *, void *);
} blysk__loop_internal_info;


#define DIVIDE(_start,_end) \
{ \
  char arg[loop->arg_size + loop->arg_align - 1 + sizeof(blysk__loop_internal_info *)]; \
  char *arg1 = arg; \
  arg1+=sizeof(blysk__loop_internal_info *); \
      if (loop->cpyfn) \
	{ \
	  loop->cpyfn (arg1, loop->data); \
	} \
      else \
	memcpy (arg1, loop->data, loop->arg_size); \
      *((void **) arg) = (void *) loop; \
      ((long *)arg1)[0] = _start; \
      ((long *)arg1)[1] = _end; \
  GOMP_task ( &BLYSK__div_and_conquer_internal , arg, NULL , loop->arg_size + sizeof(blysk__loop_internal_info *), loop->arg_align, 0 , 0 ,NULL , 0); \
 } 






/*
  * Internal Divide and conquert function for tasks
*/
void BLYSK__div_and_conquer_internal ( void *args )
{	
	blysk__loop_internal_info *loop = ((blysk__loop_internal_info **) args)[0];

	args+=sizeof(blysk__loop_internal_info *);

	int start = ((long *) args)[0];
	int end = ((long *) args)[1];

	if ( end-start == 1)
	{
		loop->fn(args);
	}
	else
	{
		DIVIDE( start , start+((end-start) >> 1));
		DIVIDE ( start + ((end-start) >> 1)  , end);
		GOMP_taskwait();
	}	
	





}


/* Entry point from application into the task-for loop */
void
GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	       long arg_size, long arg_align, unsigned flags,
	       unsigned long num_tasks, int priority,
	       long start, long end, long step)
{

  /* Ok...Lets do this. */


  // Implementation #1: Naive approach only for the IWOMP 2016 paper on iteration stress. Do not use this one as its not fully complete.
  int i;
  for (i=start; i< end ; i+=1)
  {
      char *arg = (char *) malloc(arg_size + arg_align - 1);
      if (cpyfn)
	{
	  cpyfn (arg, data);
	}
      else
	memcpy (arg, data, arg_size);
      ((long *)arg)[0] = i;
      ((long *)arg)[1] = i+1;
	
      GOMP_task ( fn , arg, NULL , arg_size,  arg_align, 0 , 0 , NULL, 0 );
  }  
  GOMP_taskwait();

  return;
/*
  blysk__loop_internal_info loop;
  loop.start = start;
  loop.end = end;
  loop.step = step;
  loop.fn = fn;
  loop.arg_size = arg_size;
  loop.arg_align = arg_align;
  loop.data = data;
  loop.cpyfn = cpyfn;



  char arg[arg_size + arg_align - 1 + sizeof(blysk__loop_internal_info *)];
  char *arg1 = arg;
  arg1+=sizeof(blysk__loop_internal_info *);
      if (cpyfn)
	{
	  cpyfn (arg1, data);
	}
      else
	memcpy (arg1, data, arg_size);
      *((void **) arg) = (void *) &loop;
      ((long *)arg1)[0] = start;
      ((long *)arg1)[1] = end;

  GOMP_task ( &BLYSK__div_and_conquer_internal , arg, NULL , arg_size + sizeof(blysk__loop_internal_info *), arg_align, 0 , 0 ,NULL , 0);
  GOMP_taskwait();*/



  

}
