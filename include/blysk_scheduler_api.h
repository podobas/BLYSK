#pragma once

#include <blysk_public.h>

#ifdef	__cplusplus
extern "C" {
#endif

size_t SCHEDULER_System_Init(blysk_icv *__restrict icv) __attribute((externally_visible, nonnull(1)));
void SCHEDULER_Thread_Init(blysk_thread *__restrict thr) __attribute((externally_visible, nonnull(1)));
void SCHEDULER_Thread_Exit(blysk_thread *__restrict thr) __attribute((externally_visible, nonnull(1)));
void SCHEDULER_Exit() __attribute((externally_visible));
void SCHEDULER_Release(blysk_thread *__restrict thr, blysk_task *__restrict task) __attribute((externally_visible, nonnull(1,2)));
blysk_task * SCHEDULER_Fetch(blysk_thread *__restrict thr) __attribute((externally_visible, nonnull(1)));
blysk_task * SCHEDULER_Blocked_Fetch(blysk_thread *__restrict thr) __attribute((externally_visible, nonnull(1)));

#ifdef	__cplusplus
}
#endif
