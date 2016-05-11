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
#include <blysk_public.h>
#include <pthread.h>
#include "../system/blysk_dep_imp.h"


#if defined(__STAT_TASK)

static blysk_task *linked_task_list = NULL, *linked_task_list_tail = NULL;
static unsigned long long region_start_time = 0;

void blysk__GG_init ( void )
{
   region_start_time = rdtsc();
   fprintf(stderr,"BLYSK Grain-Graph initialized at time %llu\n", region_start_time);   
   linked_task_list = NULL; /*Reset the linked task-list */  
   
}

void blysk__GG_add_complete_task ( blysk_task *_tsk)
{
  static pthread_mutex_t _gg_add_lock;

  _tsk->FPN[1] = NULL;
  pthread_mutex_lock ( &_gg_add_lock);
	if (linked_task_list == NULL)
	{
		linked_task_list = _tsk;
		linked_task_list_tail = _tsk;
		pthread_mutex_unlock(&_gg_add_lock);
		return;
	}
	linked_task_list_tail->FPN[1] = _tsk;
	linked_task_list_tail = _tsk;
	pthread_mutex_unlock(&_gg_add_lock);
	return;	
}

void blysk__GG_dump_task ( blysk_task *_tsk)
{

		_tsk->stat__exec_cycles = _tsk->stat__exec_end_instant - _tsk->stat__exec_cycles;
		fprintf(stderr,"%d , NA , %d, %d, %d, %d, %d, %llu, %llu, %llu, NA, %llu, %llu, \"HYENA-MEN\", NA, \"0x%lx\",",
		_tsk->stat__task,
		_tsk-> stat__parent,
		_tsk-> stat__joins_at,
		_tsk-> stat__cpu_id,
		_tsk-> stat__child_number,
		_tsk-> stat__num_children,
		_tsk-> stat__exec_cycles,
		_tsk-> stat__creation_cycles,
		_tsk-> stat__overhead_cycles,
		_tsk-> stat__queue_size,
		_tsk-> stat__create_instant - /*blysk__THREAD_get_icv()->stat__start_time*/ region_start_time,
		_tsk-> stat__exec_end_instant - /*blysk__THREAD_get_icv()->stat__start_time*/ region_start_time,
		(unsigned long) _tsk->tsk);

		fprintf(stderr,"\"[");
		for (int i=0;i<_tsk->stat__joins_counter;i++)
		{
		 fprintf(stderr,"%llu",_tsk->stat__arrive_counter[i] - /*blysk__THREAD_get_icv()->stat__start_time*/ region_start_time);
		 if (i != _tsk->stat__joins_counter-1) fprintf(stderr,",");
		}
		fprintf(stderr,"]\"\n");
              
}

void blysk__GG_exit ( void )
{
   fprintf(stderr,"BLYSK Grain-Graph exited at time %llu\n",rdtsc());
   while (linked_task_list != NULL)
     {
       blysk__GG_dump_task ( linked_task_list );
       void *to_free = linked_task_list;
       linked_task_list = linked_task_list->FPN[1];
       //freeTask(to_free);
     }
}


#endif //__STAT_TASK




