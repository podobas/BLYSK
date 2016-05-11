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

void
GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	       long arg_size, long arg_align, unsigned flags,
	       unsigned long num_tasks, int priority,
	       long start, long end, long step)
{
  /* from start ... end with +step */
  
  
  int i;
  //  fprintf(stderr,"Iter: %d -> %d\tStep: %d\n",start,end, step);
  
  for (i=start; i<end ;i++)
    {

      char arg[arg_size + arg_align - 1];
      if (cpyfn)
	{
	  cpyfn (arg, data);
	}
      else
	memcpy (arg, data, arg_size);

      //  fprintf(stderr,"%d ->",i); 
      ((long *)arg)[0] = i;
      ((long *)arg)[1] = i+1;
      fn(arg);
    }
}
