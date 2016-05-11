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

#include <memory.h>
#include <blysk_memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#if defined(__NUMA)
#include <numa.h>
#endif

mem_list *tail = NULL, *head = NULL;
pthread_mutex_t mem_lock;
unsigned int RR = 0;
//unsigned int exp = 0;

void * blysk__MEMORY_simple(unsigned long sz) {

    unsigned int tmp = __sync_fetch_and_add(&RR, 1);
    void *al = malloc(sz);

    mem_list *l1 = (mem_list *) malloc(sizeof (mem_list));
    l1->mem_data = (blysk__memory_struct *) malloc(sizeof (blysk__memory_struct));
    l1->mem_data->adr = al;
    l1->mem_data->numa_node = -1; //tmp;
    l1->mem_data->size = sz;
    l1->mem_data->meta = NULL;
    l1->next = NULL;
    pthread_mutex_lock(&mem_lock);
    if (head == NULL) {
        tail = l1;
        head = l1;
    }
    else {
        head->next = l1;
        head = l1;
    }
    pthread_mutex_unlock(&mem_lock);
    return al;
}

blysk__memory_struct *blysk__MEMORY_find(void *adr) {
    pthread_mutex_lock(&mem_lock);
    mem_list *t = tail;
    while (t != NULL) {
        if (t->mem_data->adr == adr) {
            pthread_mutex_unlock(&mem_lock);
            return t->mem_data;
        }
        t = t->next;
    }
    pthread_mutex_unlock(&mem_lock);
    return NULL;
}

void blysk__MEMORY_add(void *adr, unsigned long sz) {
    mem_list *l1 = (mem_list *) malloc(sizeof (mem_list));
    l1->mem_data = (blysk__memory_struct *) malloc(sizeof (blysk__memory_struct));
    l1->mem_data->adr = adr;
    l1->mem_data->numa_node = -1;
    l1->mem_data->size = sz;
    l1->mem_data->meta = NULL;
    l1->next = NULL;
    pthread_mutex_lock(&mem_lock);
    if (head == NULL) {
        tail = l1;
        head = l1;
    }
    else {
        head->next = l1;
        head = l1;
    }
    pthread_mutex_unlock(&mem_lock);

}

void * blysk__MEMORY_shape(unsigned long sz, int m1, int m2, int m3, int m4,
        int d1, int d2, int d3, int d4) {
    fprintf(stderr, "Allocating Complex: ");
    fprintf(stderr, "%dx%d  of size %dx%d\n", m1, m2, d1, d2);
    void *al = malloc(sz);
    float *ax = (float *) al;

    int i, ii;
    for (i = 0; i < m2; i++) //y-dir
    {
        for (ii = 0; ii < m1; ii++) //x-dir
        {
            blysk__MEMORY_add((void *) ax, d1 * d2 * 4);
            ax += d1;
        }
        ax += (m1 * (d1 - 1)) * d2;
    }
    return al;
}

