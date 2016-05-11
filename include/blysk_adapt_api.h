#pragma once 

/* This header file outlines the interface blysk expects from adaptivity
 * libraries. */

#ifdef	__cplusplus
extern "C" {
#endif

void* ADAPT_Main(void*)  __attribute((externally_visible));
void ADAPT_Kill()  __attribute((externally_visible));

#ifdef	__cplusplus
}
#endif
