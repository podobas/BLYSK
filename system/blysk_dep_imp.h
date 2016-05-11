#pragma once

#include <blysk_public.h>
#include <utils.h>
#include <blysk_malloc.h>
#include <blysk_dep.h>

#include "blysk_dep_ds_imp.h" // Implements the data structure managing dependencies

/// Dependency manager implementation and its synchronization

static inline void blysk__DEP_init(blysk_task *_task) {
    _task->__dep_manager = (blysk_bit_table *) malloc(sizeof (blysk_bit_table)); // TODO alloc?
    blysk_bit_table *TABLE = (blysk_bit_table *) _task->__dep_manager;
    for (int i = 0; i < 64; i++) TABLE->NODES[i] = NULL;
    TABLE->S_Lock = 0x0;
#if BLYSK_ENABLE_BOOKMARKS != 0
    TABLE->bookmark_id_count = 0;
    TABLE->bookmark_session = TRANSFER_NO;
#endif // ENABLE_BOOKMARKS != 0
    pthread_mutex_init(&TABLE->q_lock, NULL);
#if defined(BLYSK_DEP_SLR_LOCK)
    tasInit(&TABLE->s_lock);
#endif
    __sync_synchronize();
}

/* Without bookmarks */

#if defined(BLYSK_DEP_STRIPED_LOCK)
#define acquireQLock() {\
s0: if ((TABLE->S_Lock & L_Lock) != 0) { __sync_synchronize(); goto s0; } \
 pthread_mutex_lock(&TABLE->q_lock);\
     if ((TABLE->S_Lock & L_Lock) != 0) {pthread_mutex_unlock(&TABLE->q_lock); \
     goto s0;}\
     TABLE->S_Lock |= L_Lock;\
 pthread_mutex_unlock(&TABLE->q_lock);\
}
#define releaseQLock() pthread_mutex_lock(&TABLE->q_lock); TABLE->S_Lock &= ~L_Lock; pthread_mutex_unlock(&TABLE->q_lock)
#elif defined(BLYSK_DEP_SLR_LOCK)
#define acquireQLock() slrLock(&TABLE->s_lock, 2)
#define releaseQLock() slrUnlock(&TABLE->s_lock)
#elif defined(BLYSK_DEP_GLOBAL_LOCK)
#define acquireQLock() pthread_mutex_lock(&TABLE->q_lock)
#define releaseQLock() pthread_mutex_unlock(&TABLE->q_lock)
#elif defined(BLYSK_DEP_CAS_STRIPED_LOCK)
#define acquireQLock() {\
    unsigned long long old = TABLE->S_Lock;\
    do {\
        while(expectFalse((old & L_Lock)  != 0)) {\
            old = TABLE->S_Lock;\
        }\
    } while(expectFalse(!__atomic_compare_exchange_n(&TABLE->S_Lock, &old, old | L_Lock, true, memory_order_acquire, memory_order_relaxed)));\
}
#define releaseQLock() __atomic_sub_fetch(&TABLE->S_Lock, L_Lock, memory_order_release)
#else
#error Need to define M_LOCK, G_LOCK, or C_LOCK
#endif

static inline void freenode(blysk_bit_node*n) {
    if (n == 0) {
        return;
    }
    freenode(n->NS[0]);
    freenode(n->NS[1]);
    dealloc(n->data, sizeof (blysk_bit_node));
    dealloc(n, sizeof (blysk_dep_object));
}

static inline void freeDepManager(blysk_bit_table* t) {
    if (t == 0) {
        return;
    }
    for (int i = 0; i != 64; i++) {
        freenode(t->NODES[i]);
    }
    // TODO unsafe?
    //    dealloc(t, sizeof(blysk_bit_table)); 
}

static inline void freeTask(blysk_task* _task) {
    assert(_task->_udependencies.count == 0);
    freeDepManager(_task->__dep_manager);
    dealloc(_task, _task->size);
}

/** Release the finished _task dependencies, and schedule tasks if they are now able to run */
static inline void blysk__DEP_release(blysk_task *_task) {
    if (_task->_dependencies == 0) {
#if defined(__STAT_TASK)
      	// Delay task de-allocation
#else
        freeTask(_task);
#endif
        return;
    }

    /* Find parents TABLE */
    blysk_task *new_task_parent = (blysk_task *) _task->_parent;
    blysk_bit_table *TABLE = new_task_parent->__dep_manager;
    unsigned long long L_Lock = _task->__dep_LOCK_mask;

    acquireQLock();
    for (unsigned iter = 0; iter < _task->_dependencies; iter++)
        if (_task->dependency[iter].m != DEP_UNUSED) blysk__DEP_remove(_task->dependency[iter].o, _task->dependency[iter].m == DEP_READ ? 1 : -1);
    __sync_synchronize();
    releaseQLock();
}

#if BLYSK_ENABLE_BOOKMARKS != 0
static inline void DECODE(unsigned char** streampp, blysk_task * restrict _task, blysk_bit_table * restrict TABLE) {
    unsigned char *decode_val = *streampp;
    int ndeps = (*decode_val) >> 2;
    _task->_udependencies = ndeps;
    _task->_dependencies = ndeps;
    allocDeps(_task, ndeps);
    if ((*decode_val & 0x3) == M_8BIT) {
        decode_val++;
        unsigned char *decode_8bit = (unsigned char *) decode_val;
        int ij;
        for (ij = 0; ij < ndeps; ij++) {
            _task->_udependencies -= blysk_DEP_modify(TABLE, TABLE->REUSE_NODES[ (*decode_8bit) >> 2], _task, (*decode_8bit) & 0x3, ij, 0x0);
            decode_8bit++;
        }
        *streampp = decode_8bit;
    }
    else {
        decode_val++;
        unsigned short *decode_16bit = (unsigned short *) decode_val;
        int ij;
        for (ij = 0; ij < ndeps; ij++) {
            _task->_udependencies -= blysk_DEP_modify(TABLE, TABLE->REUSE_NODES[ (*decode_16bit) >> 2], _task, (*decode_16bit) & 0x3, ij, 0x0);
            decode_16bit++;
        }
        *streampp = decode_16bit;
    }
}
#endif // ENABLE_BOOKMARKS != 0

/** Register the tasks dependencies in its parent, and removes any satisfied dependencies */
static inline char blysk__DEP_solve(blysk_task *_task, char **_indeps, int _n_indeps,
        char **_outdeps, int _n_outdeps,
        char **_inoutdeps, int _n_inoutdeps,
        char *bookmark) { // TODO why are we returning char??
    assume(BLYSK_ENABLE_BOOKMARKS || bookmark == 0);
    blysk_bit_table *TABLE = blysk__THREAD_get_task()->__dep_manager; /* Find parents TABLE */
    int iter;
    /* Create the micro-lock */
    unsigned long long L_Lock = 0;

#if BLYSK_ENABLE_BOOKMARKS != 0
    /* Bookmark guidance */
    if (bookmark != NULL) {
        if (_task->__dep_manager == NULL) blysk__DEP_init(_task);
        blysk_bit_table *TABLE_NTASK = (blysk_bit_table *) _task->__dep_manager;

        if (transfer == NULL) {
            transfer = Transfer__NewEntry(bookmark);
            TABLE_NTASK->bookmark_session = TRANSFER_RECORD; //Start recording
        }
        else {
            /* Temporary. Compress the bookmark first and the re-use it. */
            fprintf(stderr, "Re-using %s\n", bookmark);
            Transfer__CompressAndManage(transfer);

            transfer->transfer_stream += 2; /* Adjust for reservation stations. Fix this! */
            transfer->req_res_station = *((unsigned short *) &transfer->transfer_stream[-2]);
            fprintf(stderr, "Required-Reservation-Stations: %d\n", *((unsigned short *) &transfer->transfer_stream[-2]));
            TABLE_NTASK->bookmark_session = TRANSFER_REUSE;
            TABLE_NTASK->REUSE_NODES = (blysk_bit_node **) malloc(sizeof (blysk_bit_node *) * transfer->req_res_station);
            unsigned i;
            for (i = 0; i < transfer->req_res_station; i++)
                TABLE_NTASK->REUSE_NODES[i] = newnode(); //NULL;

            fprintf(stderr, "Re-use initialization complete...\n");
        }
    }

    if (TABLE->bookmark_session == TRANSFER_RECORD)
        Transfer__RecordTaskHeader(transfer, _ta// Implements the dependency manager and its synchronizationsk->_dependencies);

    /* Bookmark in session */
    if (TABLE->bookmark_session == TRANSFER_REUSE) {
        unsigned long long *lck = (unsigned long long *) transfer->transfer_stream;
        L_Lock = *lck++;
        transfer->transfer_stream = (unsigned char *) lck;
        /* Save the lock for when we release dependencies */
        _task->__dep_LOCK_mask = L_Lock;

        acquireQLock();
        DECODE(&transfer->transfer_stream, _task, TABLE);
        //		 __sync_synchronize();

        char bdep = _task->_udependencies;
        releaseQLock();

        return bdep;
    }
#endif // ENABLE_BOOKMARKS

    // Determine which dependency tries we will update
    for (iter = 0; iter < _n_indeps; iter++) L_Lock |= ADR(((unsigned long) _indeps[iter]));
    for (iter = 0; iter < _n_outdeps; iter++) L_Lock |= ADR(((unsigned long) _outdeps[iter]));
    for (iter = 0; iter < _n_inoutdeps; iter++) L_Lock |= ADR(((unsigned long) _inoutdeps[iter]));
    _task->__dep_LOCK_mask = L_Lock; /* Save the lock for when we release dependencies */

    acquireQLock();
#if defined(SOFTWARE_CACHE)
    blysk_bit_node *nd;
    for (iter = 0; iter < _n_indeps; iter++) {
        _task->dependency[iter].m = DEP_READ;
        nd = Check_Cache((unsigned long) _indeps[iter]);
        if (nd != NULL)
            _task->_udependencies -= blysk_DEP_modify(nd, _task, DEP_READ, iter, (unsigned long) _indeps[iter]);
        else
            _task->_udependencies -= blysk__DEP_BIT_add(TABLE, (unsigned long) _indeps[iter], _task, 1, iter);
    }

    for (iter = 0; iter < _n_outdeps; iter++) {
        _task->dependency[iter + _n_indeps].m = DEP_WRITE;
        nd = Check_Cache((unsigned long) _outdeps[iter]);
        if (nd != NULL)
            _task->_udependencies -= blysk_DEP_modify(nd, _task, DEP_WRITE, iter + _n_indeps, (unsigned long) _outdeps[iter]);
        else
            _task->_udependencies -= blysk__DEP_BIT_add(TABLE, (unsigned long) _outdeps[iter], _task, -1, iter + _n_indeps);
    }


    for (iter = 0; iter < _n_inoutdeps; iter++) {
        _task->dependency[iter + _n_indeps + _n_outdeps].m = DEP_READWRITE;
        nd = Check_Cache((unsigned long) _inoutdeps[iter]);
        if (nd != NULL)
            _task->_udependencies -= blysk_DEP_modify(nd, _task, DEP_READWRITE, iter + _n_indeps + _n_outdeps, (unsigned long) _inoutdeps[iter]);
        else
            _task->_udependencies -= blysk__DEP_BIT_add(TABLE, (unsigned long) _inoutdeps[iter], _task, -1, iter + _n_indeps + _n_outdeps);
    }

#else
    for (iter = 0; iter < _n_outdeps; iter++) {
        _task->dependency[iter + _n_indeps].m = DEP_WRITE;
        _task->_udependencies.count -= blysk__DEP_BIT_add(TABLE, (unsigned long) _outdeps[iter], _task, -1, iter + _n_indeps);
    }
    for (iter = 0; iter < _n_inoutdeps; iter++) {
        _task->dependency[iter + _n_indeps + _n_outdeps].m = DEP_READWRITE;
        _task->_udependencies.count -= blysk__DEP_BIT_add(TABLE, (unsigned long) _inoutdeps[iter], _task, -1, iter + _n_indeps + _n_outdeps);
    }
    for (iter = 0; iter < _n_indeps; iter++) {
        _task->dependency[iter].m = DEP_READ;
        _task->_udependencies.count -= blysk__DEP_BIT_add(TABLE, (unsigned long) _indeps[iter], _task, 1, iter);
    }
#endif // define(SOFTWARE_CACHE)
    //    __sync_synchronize();
    char bdep = _task->_udependencies.count; // TODO why are we using char??
    releaseQLock();
    if (bdep < MIN_DEP) {
        // TODO notice that the task is almost ready
        cq_enqueue(_task);
    }
    return bdep;
}
