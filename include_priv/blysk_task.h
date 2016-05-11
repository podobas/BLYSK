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

#include <blysk_public.h>
#include <blysk_malloc.h>

/** Allocate a task with a given number of dependencies and size of arguments */
static inline blysk_task* allocTask(size_t nDependencies, u32 argSize, u32 arg_align) {
    size_t size = sizeof (blysk_task) + sizeof (blysk_dep_data) * (nDependencies) + argSize;
    if(arg_align > sizeof(void*)) {
        size += arg_align - sizeof(void*);
    }
    blysk_task* t = (blysk_task *) alloc(size);
#if BLYSK_ENABLE_DATASPEC != 0
    t->self = 0;
#endif
    t->size = size;
    t->args = nDependencies + t->dependency;
    t->args =  (void*)((arg_align - 1 + (size_t)t->args) & ~((size_t)(arg_align) - 1));
    assume(((size_t)t->args & (arg_align - 1)) == 0);
    return t;
}

/** Returns the task arguments */
static inline void* getTaskArgs(blysk_task* task) {
    // The task arguments are stored after the dependencies.
    assume(0 == ((sizeof(void*) - 1) & (size_t)(task->args)));
    return (void*)task->args;
//    return (void*) (task->_dependencies + task->dependency);
}
