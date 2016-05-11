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
#include <math.h>
#include <pthread.h>
#include <blysk_task.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define B_SIZE (100000)

typedef struct {
  blysk_task *Hall[B_SIZE];
  long Hall_priority[B_SIZE];

  unsigned int __elements;
  pthread_mutex_t Glock;
} PQueue;


static inline PQueue * PQueue_New ( void )
{
   PQueue *q = (PQueue *) malloc(sizeof(PQueue));
   if (q == NULL) {fprintf(stderr,"Failed allocating queue\n");abort();}
   q->__elements = 0; pthread_mutex_init (&q->Glock , NULL ); __sync_synchronize();
   return q;
}

static inline void PQueue_Enqueue_Back ( PQueue *q , blysk_task *_tsk, long value)
{
   pthread_mutex_lock (&q->Glock);

        q->Hall_priority[ q->__elements] = value;
        q->Hall[ q->__elements] = _tsk;
	q->__elements++;
	if (q->__elements != 1)
	{
	  unsigned int parent,child = q->__elements - 1;    
	  while (1)
	    {
	      parent = floor((child-1) >> 1) ;
	      if (q->Hall_priority[parent] > q->Hall_priority[child])
		{
		  blysk_task *tmp = q->Hall[child];
		  q->Hall[child] = q->Hall[parent];
		  q->Hall[parent] = tmp;
		  long tmp2 = q->Hall_priority[child];
		  q->Hall_priority[child] = q->Hall_priority[parent];		  
		  q->Hall_priority[parent] = tmp2;		  
		}
	      else
		break;
	      child = parent;
	      if (child == 0) break;
	      }    
        } 
//   __sync_synchronize();
   pthread_mutex_unlock ( &q->Glock);
}

static inline void PQueue_Enqueue_Front ( PQueue *q , blysk_task *_tsk, long value)
{
   PQueue_Enqueue_Back ( q, _tsk , value);
}

static inline blysk_task *PQueue_Pop_Front ( PQueue *q )
{
    pthread_mutex_lock (&q->Glock);
    if (q->__elements == 0)
	{	   
	    pthread_mutex_unlock (&q->Glock);
	    return NULL;	    
	}

    blysk_task *tmp = q->Hall[0];
    q->Hall[0] = q->Hall[ q->__elements-1 ];
    q->Hall_priority[0] = q->Hall_priority[ q->__elements-1 ];
    q->__elements--;

    if (q->__elements > 1)
       {
	 unsigned int parent = 0;
	 unsigned int swap_child;
	 while(1)
	   {
	     swap_child = parent;
	     
	     unsigned int child = (parent << 1) + 1;
	     if ( child >= q->__elements ) break;
	     if (q->Hall_priority[child] < q->Hall_priority[parent] )
		swap_child = child;

	     child = (parent << 1) + 2;	     
	     if (!(child >= q->__elements))
	       {
		 if (q->Hall_priority[child] < q->Hall_priority[swap_child])
		   swap_child = child;		  
	       }
	     
	     if (swap_child == parent) break;

	     blysk_task *tmp = q->Hall[swap_child];
	     q->Hall[swap_child] = q->Hall[parent];
	     q->Hall[parent] = tmp;
	     long tmp2 = q->Hall_priority[swap_child];
	     q->Hall_priority[swap_child] = q->Hall_priority[parent];		  
	     q->Hall_priority[parent] = tmp2;		  
	     parent = swap_child;
	     }
       }

//   __sync_synchronize();
    pthread_mutex_unlock ( &q->Glock);
    return tmp;
}

static inline blysk_task *PQueue_Pop_Back ( PQueue *q )
{
    blysk_task *tmp = NULL;
    pthread_mutex_lock (&q->Glock);
    if (q->__elements >= 1)
	{
	    tmp = q->Hall[q->__elements-1]; q->__elements--;
	}
//   __sync_synchronize();
    pthread_mutex_unlock ( &q->Glock);
    return tmp;
}

#ifdef __cplusplus 
}
#endif
