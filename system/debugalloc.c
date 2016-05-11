#include <string.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <utils.h>
#include <pageAlloc.h>

static inline size_t getSize(void* data) {
    return *((size_t*) (((char*) data) - 16));
}

static inline size_t getOrigSize(void* data) {
    return *((size_t*) (((char*) data) - 8));
}

static inline void* setSize(void* data, size_t size, size_t osize) {
    assert((size & (PAGE_SIZE - 1)) == 0);
    assert((((uptr) data) & (PAGE_SIZE - 1)) == 0);
    *((size_t*) data) = size;
    *((size_t*) (8 + (char*) data)) = osize;
    return (void*) (((char*) data) + 16);
}

#ifndef NDEBUG
static inline
#endif
void freeB(size_t osize, size_t size, void* data) {

}

#ifndef NDEBUG
static inline
#endif
void allocB(size_t osize, size_t size, void* data) {

}

FA_ALLOC_SIZE(1) FA_MALLOC FA_RETURNS_NONNULL FA_WARN_UNUSED_RESULT FA_ASSUME_ALIGNED(PAGE_SIZE, 0)
void* malloc(size_t size) {
    size_t osize = size;
    size += 16;
    void* data = pageAlooc(&size);
    //    memset(data, 0x5b, size);
    allocB(osize, size, data);
    return setSize(data, size, osize);
}

FA_ALLOC_SIZE(1, 2) FA_MALLOC FA_RETURNS_NONNULL FA_WARN_UNUSED_RESULT FA_ASSUME_ALIGNED(PAGE_SIZE, 0)
void* calloc(size_t num, size_t size) {
    size *= num;
    void* data = malloc(size);
    memset(data, 0, size);
    return data;
}

void free(void* data) {
    if (data == 0) {
        return;
    }
    size_t size = getSize(data);
    size_t origSize = getOrigSize(data);
    freeB(origSize, size, data);
    compilerMemBarrier();
    assert((size & (PAGE_SIZE - 1)) == 0);
    munmap(data, size);
}

FA_WARN_UNUSED_RESULT FA_ASSUME_ALIGNED(PAGE_SIZE, 0)
void* realloc(void *data, size_t newSize) {
    if (data == 0) {
        return malloc(newSize);
    }
    if (newSize == 0) {
        free(data);
        return 0;
    }
    size_t size = getSize(data);
    void* newData = malloc(newSize);
    size_t cpySize = size - 16;
    if (newSize < cpySize) {
        cpySize = newSize;
    }
    memcpy(newData, data, cpySize);
    free(data);
    return newData;
}
