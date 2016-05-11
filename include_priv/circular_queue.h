#pragma once 

#ifdef	__cplusplus
extern "C" {
#endif

#if BLYSK_ENABLE_DATASPEC
static inline void cq_enqueue(blysk_task* t) {
    blysk__thread->cq_elem[blysk__thread->cq_back] = t;
    blysk__thread->cq_back++;
    if(blysk__thread->cq_back == CQ_CAP) {
        blysk__thread->cq_back = 0;
    }
}

static inline blysk_task* cq_peek() {
    return blysk__thread->cq_elem[blysk__thread->cq_front];
}

static inline void cq_skip() {
    if(blysk__thread->cq_front == CQ_CAP) {
        blysk__thread->cq_front = 0;
    }
}

static inline blysk_task* cq_dequeue() {
    blysk_task* r = blysk__thread->cq_elem[blysk__thread->cq_front];
    blysk__thread->cq_elem[blysk__thread->cq_front] = 0;
    cq_skip();
    return r;
}
#else

static inline void cq_enqueue(blysk_task* t) { }
static inline blysk_task* cq_peek() { return 0; }
static inline void cq_skip() { }
static inline blysk_task* cq_dequeue() { return 0; }
#endif

#ifdef	__cplusplus
}
#endif

