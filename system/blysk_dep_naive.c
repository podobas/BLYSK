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
#include <blysk_dep.h>
#include <arch.h>
#include <string.h>
#include <blysk_thread.h>
#include <pthread.h>
#include <blysk_trace.h>

//#define G_LOCK 1
#define M_LOCK 1

#define ADR(x) (1 << (((x>>2) & 0x3f)))

unsigned int o_lock = 0;
pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;

//blysk_bit_table BIT_Table;

volatile unsigned long long S_Lock = 0x0;



typedef struct {
  void *next;
  blysk_bit_node *node;
  unsigned long adr;
} naive_list;

naive_list *TABLE = NULL, *TABLE_END = NULL;


inline blysk_dep_entry *New_Entry ( void *tsk, unsigned char mod ) 
	{ blysk_dep_entry *tmp =  (blysk_dep_entry *) malloc(sizeof(blysk_dep_entry)); if (tmp==NULL) {fprintf(stderr,"Failed allocating new entry\n");abort();}
	  tmp->Next = NULL;tmp->_task = tsk; tmp->_modifier = mod; return tmp; }


inline blysk_bit_node * newnode( void )
{
  blysk_bit_node * tmp = (blysk_bit_node *) malloc(sizeof(blysk_bit_node));
  tmp->NS[0] = NULL; tmp->NS[1] = NULL; 
  tmp->data = NULL; tmp->active = 0;
  return tmp;
}

char blysk_DEP_modify ( blysk_bit_node * tau, blysk_task *_task , unsigned char modifier , unsigned int tsk_dep_nr, unsigned long adr)
{

     if (tau->data == NULL)
	{
	        blysk_dep_object *ndata =  (blysk_dep_object*) malloc(sizeof(blysk_dep_object));
		if (ndata == NULL) {fprintf(stderr,"Failed allocate dependency object\n");abort();}
	        ndata->QTail = NULL;ndata->QHead = NULL;
	        ndata->active_gen = 0;ndata->queue_gen = 0;
	        ndata->active_task_playground = 1; ndata->active_modifier = modifier;
		ndata->adress = adr;
	        tau->data = ndata;
		tau->data->owner_tau = tau;
		tau->data->_bookmark = NULL;
		 _task->dependency[tsk_dep_nr].o = ndata;
		tau->data->_memory = blysk__MEMORY_find ( (void *) adr);
		//if (tau->data->_memory == NULL) fprintf(stderr,"Failed to find address\n");
		return 1;
	}

   tau->data->owner_tau = tau; //Temp
   blysk_dep_object *loc = tau->data;

   if ( loc->active_task_playground == 0 && 
	loc->QHead == loc->QTail)
	  {	loc->active_task_playground = 1;
		loc->active_modifier = modifier; 
		_task->dependency[tsk_dep_nr].o = loc;
		return 1; }


   if ( loc->active_gen == loc->queue_gen &&
	loc->active_modifier == modifier &&
	modifier == DEP_READ)
	     { loc->active_task_playground ++; _task->dependency[tsk_dep_nr].o = loc;return 1;}
   
   blysk_dep_entry * NewE = New_Entry ( _task , modifier);
   if (loc->QHead == NULL) 
	{loc->QHead = NewE; loc->QTail = NewE; } else { loc->QHead->Next = NewE;  loc->QHead = NewE; }

   loc->queue_gen++;
   _task->dependency[tsk_dep_nr].o = loc;  
  return 0;  
}

/* Naive linked list approach */
char blysk_DEP_BIT_find_add ( unsigned long adr , blysk_task *_task, unsigned char modifier , unsigned int tsk_dep_nr )
{

   if (TABLE == NULL)
	{
	   TABLE = (naive_list *) malloc(sizeof(naive_list));
	   TABLE->adr = adr;
	   TABLE->node = newnode();
	   TABLE->next = NULL;
	   TABLE_END = TABLE;
	   return blysk_DEP_modify ( TABLE->node , _task , modifier, tsk_dep_nr, adr);	   
	}

    naive_list *iter = TABLE;
    while (iter != NULL)
    {
	if (iter->adr == adr)
	  { return blysk_DEP_modify ( iter->node , _task , modifier, tsk_dep_nr, adr); }
	iter = (naive_list *) iter->next;
    }
   iter = (naive_list *) malloc(sizeof(naive_list));
   iter->adr = adr;
   iter->node = newnode();
   iter->next = NULL;
   TABLE_END->next = iter;
   TABLE_END = iter;
   return blysk_DEP_modify ( iter->node , _task , modifier, tsk_dep_nr, adr);	   
   
}

char blysk__DEP_BIT_add ( unsigned long adr , blysk_task *_task, unsigned char modifier , unsigned int tsk_dep_nr)
{

    return blysk_DEP_BIT_find_add ( adr , _task , modifier , tsk_dep_nr );
}



void blysk__DEP_init (void)
  { }

char  blysk__DEP_solve ( blysk_task *_task , char **_indeps,    int _n_indeps, 
					     char **_outdeps,   int _n_outdeps,
					     char **_inoutdeps, int _n_inoutdeps)
{

    int iter;

    unsigned long long L_Lock = 0;
    for (iter = 0 ; iter < _n_indeps; iter++)  L_Lock |= ADR(((unsigned long) _indeps[iter]));
    for (iter = 0 ; iter < _n_outdeps; iter++) L_Lock |= ADR(((unsigned long) _outdeps[iter]));
    for (iter = 0 ; iter < _n_inoutdeps; iter++) L_Lock |= ADR(((unsigned long) _inoutdeps[iter]));

    _task->_odependencies = (void **) malloc(sizeof(void*) * _task->_dependencies );
    if (_task->_odependencies == NULL) {fprintf(stderr,"Failed allocating o_dependencies\n");abort();}


#if defined(G_LOCK)
    pthread_mutex_lock(&q_lock);
#endif

#if defined(M_LOCK)
 s0: if ((S_Lock & L_Lock) != 0) {__sync_synchronize(); goto s0;}
 s1: pthread_mutex_lock(&q_lock);
     if ((S_Lock & L_Lock) != 0) {pthread_mutex_unlock(&q_lock); __sync_synchronize();goto s0;}
     S_Lock |= L_Lock;
     __sync_synchronize();
 pthread_mutex_unlock(&q_lock);
#endif


#if defined(S_LOCK)
 unsigned long L1 = L_Lock, L1_tmp, T1 = L1 & (1 <<  __builtin_ctz ( L1 ));
 s0: L1_tmp = __sync_fetch_and_or ( &S_Lock , T1 ); 
    if ( (L1_tmp & T1) != T1)
	{  L1 = L1 & (~T1);
	   if (L1 == 0) goto s1;
	   T1 = L1 & (1 <<  __builtin_ctz ( L1 ));
	}
    goto s0;
 s1: ;
#endif

    _task->_mdependencies = (char *) malloc(sizeof(char) * _task->_dependencies );
    _task->_rdependencies = (unsigned int *) malloc(sizeof(char) *_task->_dependencies);

     for (iter=0;iter<_task->_dependencies;iter++) _task->_rdependencies[iter] = -1;
    _task->_mdependencies[iter+_n_indeps] = DEP_WRITE;
    _task->_mdependencies[iter+_n_indeps+_n_outdeps] = DEP_READWRITE;

    for (iter = 0 ; iter < _n_indeps; iter++) {
	   _task->_mdependencies[iter] = DEP_READ;
	   _task->_udependencies -= blysk__DEP_BIT_add ( (unsigned long) _indeps[iter] , _task, DEP_READ , iter); }

    for (iter = 0 ; iter < _n_outdeps; iter++) {
	   _task->_mdependencies[iter+_n_indeps] = DEP_WRITE;
	   _task->_udependencies -= blysk__DEP_BIT_add ( (unsigned long) _outdeps[iter] ,_task, DEP_WRITE , iter+_n_indeps); }

    for (iter = 0 ; iter < _n_inoutdeps; iter++) {
	   _task->_mdependencies[iter+_n_indeps+_n_outdeps] = DEP_READWRITE;
	   _task->_udependencies -= blysk__DEP_BIT_add ( (unsigned long) _inoutdeps[iter] ,_task, DEP_READWRITE , iter + _n_indeps + _n_outdeps);}

    __sync_synchronize();

    char bdep = _task->_udependencies;

#if defined(S_LOCK)
    __sync_fetch_and_and ( &S_Lock , ~L_Lock);
#endif

#if defined(M_LOCK)
 pthread_mutex_lock(&q_lock);
      S_Lock &= ~L_Lock;
     __sync_synchronize();
 pthread_mutex_unlock(&q_lock);
#endif

#if defined(G_LOCK)
    pthread_mutex_unlock(&q_lock);
#endif

    return bdep;
}


void blysk__DEP_remove ( blysk_dep_object *loc , unsigned char depRemove )
{
   if ( loc == NULL)
	{ fprintf(stderr,"Unexpected error. Object magically dissappeared...\n"); abort();}

   loc->active_task_playground --;
   if ( loc->active_task_playground == 0)
     {	
	if (loc->QHead == NULL || loc->QTail == NULL)  return;

	blysk_dep_entry *next = loc->QTail;

	blysk_task *r_task;

        loc->active_gen++;
        loc->active_modifier = next->_modifier;

	label_next:
	      r_task = next->_task; 
	      loc->active_task_playground++;
	      unsigned int rest = __sync_fetch_and_sub ( &r_task->_udependencies , 1); 
	      r_task->_rdependencies[rest] = blysk__THREAD_get_rid();
 	      if (rest == 1) 
		{
		  __sync_fetch_and_add(&r_task->_udependencies , r_task->_dependencies);
		  blysk_icv *icv = blysk__THREAD_get_icv ();
		  #if defined(__TRACE)
		     r_task->__trace_release_time = wall_time();
		     r_task->__trace_releaser = blysk__THREAD_get_rid();
		  #endif
		  r_task->__mod_released = depRemove;
		  //~ icv->scheduler.scheduler_release ( blysk__THREAD_get() , r_task );
		  scheduler_release ( blysk__THREAD_get() , r_task );

		}


		blysk_dep_entry *next2 = (blysk_dep_entry *) loc->QTail->Next;

		if (next2 != NULL &&
		    loc->active_modifier == DEP_READ &&
		    next2->_modifier == DEP_READ)
			{ blysk_dep_entry *tmp = loc->QTail; 
			  loc->QTail = loc->QTail->Next; 
			  free(tmp);
			  next = loc->QTail;			
			  goto label_next;}		

		if (loc->QTail == loc->QHead ) 
		   { free(loc->QTail);  loc->QTail = NULL; loc->QHead = NULL; }
		else
		   { blysk_dep_entry *tmp = loc->QTail; loc->QTail = loc->QTail->Next; free(tmp); }		
     }
}



void blysk__DEP_release ( blysk_task *_task )
{
   int iter;  

    unsigned long long L_Lock = 0;
    for (iter = 0; iter < _task->_dependencies ; iter ++ )
	{  blysk_dep_object *t = _task->_odependencies[iter]; L_Lock |= ADR(t->adress); }


#if defined(G_LOCK)
    pthread_mutex_lock(&q_lock);
#endif
	
#if defined(M_LOCK)
 s0: if ((S_Lock & L_Lock) != 0) {__sync_synchronize(); goto s0;}
 s1: pthread_mutex_lock(&q_lock);
     if ((S_Lock & L_Lock) != 0) {pthread_mutex_unlock(&q_lock); __sync_synchronize();goto s0;}
     S_Lock |= L_Lock;
     __sync_synchronize();
 pthread_mutex_unlock(&q_lock);
#endif


#if defined(S_LOCK)
 unsigned long L1 = L_Lock, L1_tmp, T1 = L1 & (1 <<  __builtin_ctz ( L1 ));
 s0: L1_tmp = __sync_fetch_and_or ( &S_Lock , T1 );
     if ( (L1_tmp & T1) != T1) 
 	{  L1 = L1 & (~T1);
 	   if (L1 == 0) goto s1;
 	   T1 = L1 & (1 <<  __builtin_ctz ( L1 ));
 	}
     goto s0;
 s1: ;
#endif


   for (iter = 0; iter < _task->_dependencies ; iter ++ )
     {
	blysk__DEP_remove ( _task->_odependencies[iter] , _task->_mdependencies[iter]);
     }


    __sync_synchronize();




#if defined(S_LOCK)
 __sync_fetch_and_and ( &S_Lock , ~L_Lock);
#endif


#if defined(M_LOCK)
 pthread_mutex_lock(&q_lock);
     S_Lock &= ~L_Lock;
     __sync_synchronize();
 pthread_mutex_unlock(&q_lock);
#endif


#if defined(G_LOCK)
    pthread_mutex_unlock(&q_lock);
#endif

}






