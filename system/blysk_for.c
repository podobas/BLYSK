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

//#define SINGLE_IMPL 1




void runIterationTask ( blysk_task *tsk )
{
	unsigned long long _t1;
	blysk__iterator_info *info = (blysk__iterator_info *) tsk->args;

	if (tsk->iter_space[1] - tsk->iter_space[0] == 0) return;

	char arg[info->arg_size + info->arg_align - 1];
	if (info->cpyfn)
	{
		info->cpyfn( arg, info->data);
	}
	else
	{
		memcpy(arg , info->data, info->arg_size);
	}
	((long *) arg)[0] = tsk->iter_space[0];
	((long *) arg)[1] = tsk->iter_space[1];

	info->fn(arg);

	if (tsk->_children.count != 0)
		GOMP_taskwait();
}



#if defined(SINGLE_IMPL)
/*
 * Brooding function
 * This function decomposes "brood" functions to further parallelism.
 */
void BLYSK__brood_internal ( void *args)
{
	unsigned int num_upd = 0;
	blysk_task *_tsk = blysk__THREAD_get_task();
	blysk__iterator_info *info = (blysk__iterator_info *) args;

	int remainS,remainE;

	_repeat: ;

	remainS = (_tsk->iter_space[0] - info->start ) % info->step;
	remainE = (_tsk->iter_space[1] - info->start ) % info->step;

	if ( remainS )
	   _tsk->iter_space[0] += remainS;
	if ( remainE )
	   _tsk->iter_space[1] += remainE;

	if (_tsk->iter_space[1] == _tsk->iter_space[0]) goto _complete;

	if ( expectFalse ( _tsk->iter_space[1] - _tsk->iter_space[0] == 1 ))
	{
		runIterationTask ( _tsk ); 
		goto _complete;
	}
	else
	{
		 long start = _tsk->iter_space[0], end = _tsk->iter_space[1];
		 fetchAndAddCounter(&_tsk->_parent->_children, 1, RELAXED);
		 blysk_task *i1_task = blysk__create_brood_task ( NULL , &BLYSK__brood_internal, args, start + ((end-start) >> 1) ,end);

		 /* Dont create a new task for the right-side of the decomposition.
		  * Instead, just update the iterator space and call the function */

		 _tsk->iter_space[0] = start;
		 _tsk->iter_space[1] =  start+((end-start) >> 1);
		 blysk__SEND_Scheduler (i1_task);
		 
		 /* No need to use expensive function pointer for this...Tail-recurse instead.
		  * use 'num_upd' to keep track of the number of parential children we should reduce by.
		  */
		 num_upd++;
		 goto _repeat;		    
	}

        _complete:
		if (num_upd)
		   fetchAndAddCounter(&_tsk->_parent->_children, -num_upd, RELAXED);	

}

#else

#define OWN_FRAC 3

void BLYSK__brood_internal ( void *args)
{
	unsigned int num_upd = 0;
	blysk_task *_tsk = blysk__THREAD_get_task();
	blysk__iterator_info *info = (blysk__iterator_info *) args;

	int remainS,remainE;

	_repeat: ;

	/* Perhaps this is not an iterator? */
	if (_tsk->iter_space[1] == _tsk->iter_space[0]) goto _complete;


	if (_tsk->Type == ITERATOR)
	{
		long start = _tsk->iter_space[0], end = _tsk->iter_space[1];
		long sStart,sEnd,rStart,rEnd;
		if (_tsk->creator != blysk__THREAD_get_rid() || (end-start < (1 << OWN_FRAC)))
		{
			/* Thieves are only allowed to steal one iterations.
			   This also holds if the task contains very few iterations (hence the 1 << OWN_FRAC condition) */
			sStart = start+1;
			sEnd = end;
			rStart = start;
			rEnd = start+1;
		}
		else
		{
			/* I own the task and can therefor steal a larger chunkie */
		 	rStart = start ;
		 	rEnd =  start + ((end-start) >> OWN_FRAC);
		 	sStart = start + ((end-start) >> OWN_FRAC);
		 	sEnd = end;
		}		

		/* Please dont spawn empty tasks...*/
		if (sStart != sEnd)
		{
		 	fetchAndAddCounter(&_tsk->_parent->_children, 1, RELAXED);
		 	blysk_task *i1_task = blysk__create_brood_task ( NULL , &BLYSK__brood_internal, args, sStart  , sEnd );

			 /* Promote (or demote, depending on your flavour) the task to an iteration task */
			 i1_task->Type = ITERATOR;
		 	/* Dont create a new task for the right-side of the decomposition.
		 	 * Instead, just update the iterator space and call the function */

		 	/* Execute the iteration task */
		 	blysk__SEND_Scheduler (i1_task);
		 	num_upd++;
		}		 
		_tsk->iter_space[0] = rStart;
		_tsk->iter_space[1] = rEnd;

		runIterationTask (_tsk);
		 goto _complete;	

	}


	remainS = (_tsk->iter_space[0] - info->start ) % info->step;
	remainE = (_tsk->iter_space[1] - info->start ) % info->step;

	/* Adjust start and end iterators */
	if ( remainS )
	   _tsk->iter_space[0] += remainS;
	if ( remainE )
	   _tsk->iter_space[1] += remainE;

	/* Perhaps this is not an iterator? */
	if (_tsk->iter_space[1] == _tsk->iter_space[0]) goto _complete;

	/* First things first...Are we down to a single iteration? If so, immediately execute and finish */
	if ( expectFalse ( _tsk->iter_space[1] - _tsk->iter_space[0] < 4 ))
	{
		runIterationTask ( _tsk ); 
		goto _complete;
	}

	blysk_icv *icv = blysk__THREAD_get_icv();

	/* Next question: Are we still spawning Brood tasks? If we are, then continue spawning them... */
	if ( expectTrue ( (_tsk->iter_space[1] - _tsk->iter_space[0]) >
			  ((info->end - info->start) / icv->n_Threads )  ))
	{
		 long start = _tsk->iter_space[0], end = _tsk->iter_space[1];
		 fetchAndAddCounter(&_tsk->_parent->_children, 1, RELAXED);
		 blysk_task *i1_task = blysk__create_brood_task ( NULL , &BLYSK__brood_internal, args, start + ((end-start) >> 1) ,end);

		 /* Dont create a new task for the right-side of the decomposition.
		  * Instead, just update the iterator space and call the function */
		
		 _tsk->iter_space[0] = start;
		 _tsk->iter_space[1] =  start+((end-start) >> 1);
		 blysk__SEND_Scheduler (i1_task);
		 
		 /* No need to use expensive function pointer for this...Tail-recurse instead.
		  * use 'num_upd' to keep track of the number of parential children we should reduce by.
		  */
		 num_upd++;
		 goto _repeat;		    
	}
	else /* OTHERWISE!!! We are down to iterator tasks...Take one iterator tasks and spawn the other, run it and quit */
	{
		 _tsk->Type = ITERATOR;
		_tsk->creator =  blysk__THREAD_get_rid(); 
		 goto _repeat;
	}

        _complete:
		if (num_upd)
		   fetchAndAddCounter(&_tsk->_parent->_children, -num_upd, RELAXED);	

}


#endif



/* Entry point from application into the task-for loop */
void
GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	       long arg_size, long arg_align, unsigned flags,
	       unsigned long num_tasks, int priority,
	       long start, long end, long step)
{
  blysk__iterator_info info;
  info.start = start;
  info.end = end;
  info.step = step;
  info.fn = fn;
  info.arg_size = arg_size;
  info.arg_align = arg_align;
  info.data = data;
  info.cpyfn = cpyfn;
  //info.probe = p;

  blysk_task *i1_task = blysk__create_brood_task ( blysk__THREAD_get_task() , &BLYSK__brood_internal, &info, start , start+((end-start) >> 1));
  blysk_task *i2_task = blysk__create_brood_task ( blysk__THREAD_get_task() , &BLYSK__brood_internal, &info,  start + ((end-start) >> 1)  , end);    

  blysk__SEND_Scheduler (i1_task);
  blysk__SEND_Scheduler (i2_task);


  GOMP_taskwait();


}
