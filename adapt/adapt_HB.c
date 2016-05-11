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

//#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <blysk_icv.h>

static unsigned char dead = 0;

#define PUT_SLEEP(thrid) icv->adapt_dorment[ thrid ] = 1;

#define WAKE_UP(thrid) { icv->adapt_dorment[ thrid ] = 0; \
			 syscall(__NR_futex , &icv->adapt_dorment[ thrid ], FUTEX_WAKE, 1, NULL, NULL, NULL); }



inline int count_expected_sleeps ( blysk_icv *icv)
{
  int tmp=0;
  for (unsigned i=0;i<icv->n_Threads;i++)
	tmp+= icv->adapt_dorment[i];
  return tmp;
}

inline int count_real_sleeps ( blysk_icv *icv)
{
  int tmp=0;
  for (unsigned i=0;i<icv->n_Threads;i++)
	tmp+= icv->adapt_sleep[i];
  return tmp;
}


/*
   This function is the Controller function. TBlysk will create a thread for this function.
   IT MUST BE ABORTABLE. Meaning that when ADAPT_Kill() is called, this function must exit.
*/
void ADAPT_Main (void *xarg)
{
   dead = 0;
   blysk_icv *icv = (blysk_icv *) xarg;


   while (1)
     {
	PUT_SLEEP(2);
	fprintf(stderr,"Status:\t%d Expected to be Sleep\t%d Measured at Sleep\n",count_expected_sleeps(icv) , count_real_sleeps(icv) );
	sleep(1);
	fprintf(stderr,"Status:\t%d Expected to be Sleep\t%d Measured at Sleep\n",count_expected_sleeps(icv) , count_real_sleeps(icv) );
	sleep(1);
	WAKE_UP(2);
	fprintf(stderr,"Status:\t%d Expected to be Sleep\t%d Measured at Sleep\n",count_expected_sleeps(icv) , count_real_sleeps(icv) );
	sleep(1);
	fprintf(stderr,"Status:\t%d Expected to be Sleep\t%d Measured at Sleep\n",count_expected_sleeps(icv) , count_real_sleeps(icv) );
	sleep(1);

	__sync_synchronize(); if (dead) return;
     }
}



/*
   When ADAPT_Kill() is called then the ADAPT_Main should exit!!! 
*/
void ADAPT_Kill (void)
{
   fprintf(stderr,"Killing\n");
   dead = 1; __sync_synchronize();
}
