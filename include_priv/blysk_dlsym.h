#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef void(*fptr_t)();

static inline fptr_t mydlsym(void* __restrict handle, const char*__restrict symbol, const char* __restrict handleName) {
    const char* msg = dlerror();
    void* r = dlsym(handle, symbol);
    msg = dlerror();
    if (msg != 0) {
        fprintf(stderr, "Failed to load %s from shared object %s\n%s\n", symbol, handleName, msg);
        abort();
        return 0;
    }
    //fprintf(stderr, "Loaded %p %s from shared object %s\n", r, symbol, handleName);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    return (fptr_t) r;
#pragma GCC diagnostic pop
}


#ifdef	__cplusplus
}
#endif

