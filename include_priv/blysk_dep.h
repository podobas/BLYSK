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
#include <slr_lock.h>

#define FIELD(x) ((((unsigned)x)>>2) & 0x3f)
#define ADR(x) (1ul << FIELD(x))

#define MIN_DEP (2)

#define DEP_READ 0x0
#define DEP_WRITE 0x1
#define DEP_READWRITE 0x2
#define DEP_BOOKMARKED 0x4
#define DEP_REPEAT 0x3
#define DEP_UNUSED 0x5 // TODO seems like UNUSED is unused, but checked

#define TRANSFER_NO 0x0
#define TRANSFER_RECORD 0x1
#define TRANSFER_REUSE 0x2

#if BLYSK_ENABLE_DEP_TRIE != 0
typedef struct blysk_dep_entry_t {
    blysk_task* _task;
    struct blysk_dep_entry_t *Next;
    signed char _modifier; /*< Type of dependency */
} blysk_dep_entry;
typedef struct blysk_bit_node_t blysk_bit_node;

/** Tracks tasks depending on this address. */
typedef struct blysk_dep_object_t {
    blysk_dep_entry* front; /*< The first task waiting for this data. */
    blysk_dep_entry* back; /*< The last task waiting for this data. */
    int ongoingReads; /*< The number of reading tasks currently released. -1 means there is a writing task currently released */
#ifndef NDEBUG
    blysk_bit_node* owner_tau;
    blysk_task* _last_task;
#endif
#if BLYSK_ENABLE_BOOKMARKS != 0
    void *_bookmark;
    void *_bookmark_end;
    blysk__memory_struct *_memory;
    unsigned int bookmark_id;
#endif
} blysk_dep_object;

/** Node in the bitwise binary trie */
typedef struct blysk_bit_node_t {
    uptr fast_adress; /*< Address of dependency that this node corresponds to */
    struct blysk_bit_node_t *NS[2]; /*< Left and right children */
    blysk_dep_object *data; /*< Head of the  */

    char active; /*< Non-zero if this node points to a dependency. TODO why not piggy-back on fast_address == 0 */
} blysk_bit_node;

/** Task dependency structure.
 *  Tracks the input and output dependencies of a tasks children.
 *  The dependencies are tracked/keyed based on their addresses, through the NODES.
 *  NODES form a simple (fixed size) hash map, of binary tries (which only grow), to queues of dependencies (see blysk__DEP_BIT_add).
 *  Specifically: bit_node[64] (address) => bit_node (address) => blysk_dep_object
 *  Generally: hash_map<address,binary_trie<address,forward_list<dependecies>>>
 *   */
typedef struct blysk_bit_table_t {
    volatile unsigned long long S_Lock;
    pthread_mutex_t q_lock;
#if defined(BLYSK_DEP_SLR_LOCK)
    TASLock s_lock;
#endif
    blysk_bit_node *NODES[64];
#if BLYSK_ENABLE_BOOKMARKS != 0
    blysk_bit_node **REUSE_NODES;
    char bookmark_session;
    unsigned int bookmark_id_count;
#endif
} blysk_bit_table;

#else // BITWISE
typedef struct {
    void *_task;
    unsigned char _modifier;
    void *Next;
} blysk_dep_entry;
#endif
