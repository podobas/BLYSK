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

#include <blysk_icv.h>

void blysk__SCHEDULER_new();

#ifndef BLYSK_FIXED_SCHEDULER
static inline size_t SCHEDULER_System_Init(void* a) {
 return bpc.scheduler.scheduler_system_init(a);
}
static inline void SCHEDULER_Thread_Init(void* a) {
  bpc.scheduler.scheduler_thread_init(a);
}
static inline void SCHEDULER_Thread_Exit(void* a) {
  bpc.scheduler.scheduler_thread_exit(a);
}
static inline void SCHEDULER_Exit() {
  bpc.scheduler.scheduler_exit();
}
static inline void SCHEDULER_Release(void * a, blysk_task * b) {
  bpc.scheduler.scheduler_release(a, b);
}
static inline blysk_task * SCHEDULER_Fetch(void * a) {
  return bpc.scheduler.scheduler_fetch(a);
}
static inline blysk_task * SCHEDULER_Blocked_Fetch(void *a) {
  return bpc.scheduler.scheduler_blocked_fetch(a);
}
#endif
