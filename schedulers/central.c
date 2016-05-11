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

/* Description: The following the simplest of schedulers. It is a work-dealing algorithm where a 
                central queue is used to hold att released tasks. */

#include <stdio.h>
#include <stdlib.h>
#include <blysk_scheduler_api.h>
#include <blysk_GQueue.h>


/* Declare the Central Queue as a GQueue (General-Queue) */
GQueue Central_Queue;


/* SCHEDULER_System_init: Called when a team is created. The *icv structure contains team info */
size_t SCHEDULER_System_Init (blysk_icv *icv)
{
      fprintf(stderr,"[Schedule] Central enabled...\n");
      GQueue_Init(&Central_Queue);
//      __sync_synchronize();
      return 0;
}

/* SCHEDULER_Thread_Init: Called when a thread has been added to the team. */
void SCHEDULER_Thread_Init (blysk_thread *thr)
{ }

void SCHEDULER_Thread_Exit(blysk_thread *__restrict thr) { }


void SCHEDULER_Exit ( void )
{
}


/* SCHEDULER_Release: Called when a thread wants to release a task */
void SCHEDULER_Release (blysk_thread *thr, blysk_task *task)
{
   GQueue_Enqueue_Front ( &Central_Queue , task);
}


/* SCHEDULER_Fetch: Called when a thread has nothing to do */
blysk_task * SCHEDULER_Fetch ( blysk_thread *thr)
{
  return GQueue_Pop_Front(&Central_Queue );
}


/* SCHEDULER_BLocked_Fetch: Called when a thread is blocked and is looking for work */
blysk_task * SCHEDULER_Blocked_Fetch ( blysk_thread *thr)
{
  return GQueue_Pop_Front(&Central_Queue );
}
