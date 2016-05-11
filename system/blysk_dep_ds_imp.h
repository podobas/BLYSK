#pragma once

#include <circular_queue.h>
#include <blysk_public.h>
#include <blysk_malloc.h>
#include <blysk_dep.h>

/// Implementation of the data structure managing dependencies.
/// The data structure is a map with:
/// key=addresses,          value=queued read-write dependencies
/// The map is implemented with 64 tries.
/// The trie is chosen based on bit 3-8 from the address.

/** Free a queued dependency */
static inline void free_entry(blysk_dep_entry*e) {
    dealloc(e, sizeof (blysk_dep_entry));
}

/** Allocate a queued dependency */
static inline blysk_dep_entry *New_Entry(blysk_task *tsk, unsigned char mod) {
    blysk_dep_entry *tmp = (blysk_dep_entry*) alloc(sizeof (blysk_dep_entry));
    if (tmp == NULL) {
        fprintf(stderr, "Failed allocating new entry\n");
        abort();
    }
    tmp->Next = NULL;
    tmp->_task = tsk;
    tmp->_modifier = mod;
    return tmp;
}

/** Free a trie node */
static inline blysk_bit_node * newnode(void) {
    blysk_bit_node * tmp = (blysk_bit_node*) alloc(sizeof (blysk_bit_node));
    tmp->NS[0] = NULL;
    tmp->NS[1] = NULL;
    tmp->data = NULL;
    tmp->active = 0;

    return tmp;
}

/** Release dependency loc of type depRemove, and tasks without dependencies */
static inline void blysk__DEP_remove(blysk_dep_object* loc, signed char depRemove) {
    depRemove = loc->ongoingReads == -1 ? -1 : 1;
    loc->ongoingReads -= depRemove; // Release the address
    if (loc->ongoingReads != 0 // Nothing else to do if still being read, or
            || loc->front == 0) { // No tasks waiting on this address
        assume(loc->ongoingReads >= 0);
        return;
    }
    // Release tasks waiting on this address
    blysk_dep_entry* next = loc->front;
    signed char modifier = next->_modifier;
    do {
        loc->ongoingReads += modifier;
        blysk_task *r_task = next->_task;
        unsigned int rest = fetchAndAddCounter(&r_task->_udependencies, -1, ACQUIRE) - 1;
#if defined(__TRACE)
        r_task->_rdependencies[rest] = blysk__THREAD_get_rid();
#endif

        if (rest == 0) { // release task

#if defined(__TRACE)
            r_task->__trace_release_time = wall_time();
            r_task->__trace_releaser = blysk__THREAD_get_rid();
#endif

	/* Record the release time */
	 #if defined(__STAT_TASK)
	   r_task->stat__cpu_id_release = blysk__THREAD_get_rid();
	   r_task->stat__release_instant = rdtsc();
	 #endif

            blysk__THREAD_get_icv()->scheduler.scheduler_release(blysk__THREAD_get(), r_task);
            //SCHEDULER_Release(blysk__THREAD_get(), r_task);
        }
        else if (rest < MIN_DEP) {
            // Notice that this task is about to be released
            cq_enqueue(r_task);
        }
        loc->front = next->Next;
        free_entry(next);
        if (depRemove != 1) {
            return;
        }
        next = loc->front;
        if (next == 0) {
            // loc->back = 0;
            return;
        }
        modifier = next->_modifier;
    } while (modifier != depRemove);
}

#if BLYSK_ENABLE_BOOKMARKS != 0
static transfer_entry *transfer = NULL;
#endif

#ifndef NDEBUG
/** For testing*/
static inline void noteLastTask(blysk_dep_object* loc, blysk_task* _task) {
    loc->_last_task = _task;
}
#else
#define noteLastTask(a, b) ((void)0)
#endif

/** Handle the "_task"s "modifier" dependency on the address represented by "tau->data"
 *  Returns 1 if the address is available, otherwise returns 0 */
static inline char blysk_DEP_modify(blysk_bit_table *TABLE, blysk_bit_node * tau, blysk_task *_task, signed char modifier, unsigned int tsk_dep_nr, unsigned long adr) {
    if (tau->data == NULL) { // If there are no dependencies, allocate a linked list with this dependency
        blysk_dep_object *ndata = (blysk_dep_object*) alloc(sizeof (blysk_dep_object));
        // TODO maybe store in place, rather than by reference
        ndata->front = NULL;
        ndata->back = NULL;
        ndata->ongoingReads = modifier;
        tau->data = ndata;
#ifndef NDEBUG
        tau->data->owner_tau = tau;
#endif
        _task->dependency[tsk_dep_nr].o = ndata;
        noteLastTask(ndata, _task);
#if BLYSK_ENABLE_BOOKMARKS != 0
        ndata->bookmark_id = TABLE->bookmark_id_count++;
        tau->data->_bookmark = NULL;
        /* For Memmory Support */
        /*
            tau->data->_memory = blysk__MEMORY_find ( (void *) adr);
            if (tau->data->_memory == NULL) fprintf(stderr,"Failed to find address\n"); */

        /* If we are in a recording session, then record the dependency */
        if (TABLE->bookmark_session == TRANSFER_RECORD)
            Transfer__RecordDependency(transfer, ndata->bookmark_id, modifier);
#endif // ENABLE_BOOKMARKS != 0
        return 1;
    }

    blysk_dep_object *loc = tau->data;

#if defined(SOFTWARE_CACHE)
    Add_Cache(adr, tau);
#endif

    /*if (loc->_last_task == _task) // Check for possible dead-lock
         {
                          _task->dependency[tsk_dep_nr].m = DEP_UNUSED;
                         return 1;
         }*/
#if BLYSK_ENABLE_BOOKMARKS != 0
    /* For bookmarking . Save to _DEPLIST Stream*/
    if (TABLE->bookmark_session == TRANSFER_RECORD)
        Transfer__RecordDependency(transfer, loc->bookmark_id, modifier);
#endif // ENABLE_BOOKMARKS != 0


    _task->dependency[tsk_dep_nr].o = loc;
    //_task->dependency[tsk_dep_nr].m = 2;
    if (loc->ongoingReads == 0 || // First use of address
            (loc->front == 0 && modifier == 1 && loc->ongoingReads > 0)) { // Read of address only being read
        loc->ongoingReads += modifier;
        return 1; // Keep track of the released tasks without allocating objects for them
    }

    // Record the tasks that are waiting
    blysk_dep_entry* e = New_Entry(_task, modifier);
    // Queue up for this address
    if (loc->front != 0) {
        loc->back->Next = e;
        loc->back = e;
    }
    else {
        loc->front = e;
        loc->back = e;
    }
    return 0;
}

/** Return 0 if the dependency is satisfied, otherwise add the dependency to tau, the trie of lists of dependencies */
static inline char blysk_DEP_BIT_find_add(blysk_bit_table *TABLE, unsigned long adr, blysk_task *_task, signed char modifier, unsigned int tsk_dep_nr, unsigned int bit, blysk_bit_node *tau) {
    /* Traverse the binary trie of dependencies */
again:

    if (bit == 3) bit += 5; // The hash table encodes bit 3-8, so the bit trie  skips them.
    // The branch could be avoided by peeling

    if (tau->NS[0] == tau->NS[1] && !tau->active) // TODO wouldn't !tau->active be sufficient
    { /* Leaf node without dependency. Use it */
        tau->fast_adress = adr;
        tau->active = 1;
        return blysk_DEP_modify(TABLE, tau, _task, modifier, tsk_dep_nr, adr);
    }

    if (tau->active && tau->fast_adress != adr) // TODO couldnt you reuse fast_adress for active?
    { /* Some other dependency address . Traverse to child */
        unsigned long nnb = (tau->fast_adress >> (bit - 1)) & 0x1;
        blysk_bit_node *t1 = newnode();
        tau->NS[nnb] = t1;
        t1->active = 1;
        t1->fast_adress = tau->fast_adress;
        t1->data = tau->data;
        tau->active = 0;
        tau->data = NULL;
#ifndef NDEBUG
        t1->data->owner_tau = t1;
#endif
    }

    if (tau->active && tau->fast_adress == adr) /* Matching dependency address. Add dependency to the address. */
        return blysk_DEP_modify(TABLE, tau, _task, modifier, tsk_dep_nr, adr);

    // Go to child node
    unsigned long nb = (adr >> (bit - 1)) & 0x1;
    if (tau->NS[nb] == NULL) /* If child does not exist, create it */
        tau->NS[nb] = newnode();
    bit++;
    tau = tau->NS[nb];
    goto again; /* Tail recursion */
    // return blysk_DEP_BIT_find_add ( TABLE, adr , _task , modifier , tsk_dep_nr , bit+1 , tau->NS[nb]);
}

/** Returns 0 if the dependency is satisfied, otherwise adds a dependency of the given _task to a location.
 *  TABLE is the parent tasks dependency structure.
 *  modifier is the dependency type (DEP_READ, DEP_WRITE, etc) */
static inline char blysk__DEP_BIT_add(blysk_bit_table *TABLE, unsigned long adr, blysk_task *_task, signed char modifier, unsigned int tsk_dep_nr) {
    /* Lookup the dependency in the hash table */
    int n = FIELD(adr);
    if (TABLE->NODES[n] == NULL)
        TABLE->NODES[n] = newnode();
    return blysk_DEP_BIT_find_add(TABLE, adr, _task, modifier, tsk_dep_nr, 1, TABLE->NODES[n]);
}

