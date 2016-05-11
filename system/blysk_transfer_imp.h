#pragma once

#if BLYSK_ENABLE_BOOKMARKS != 0

#include <blysk_transfer.h>

DECL transfer_entry* Transfer__NewEntry(char *name);
DECL void Transfer__RecordDependency(transfer_entry *ent, unsigned int reservation_nr, unsigned int modifier);
DECL void Transfer__CompressAndManage(transfer_entry *ent);
DECL void Transfer__RecordTaskHeader(transfer_entry *ent, unsigned int deps);


DECL void* Transfer__Analyze(void *stream, void *end, unsigned int *res);
DECL void Transfer__Encode_32bit(void **stream, unsigned int n_deps, unsigned int *deps);
DECL void Transfer__Encode_16bit(void **stream, unsigned int n_deps, unsigned int *deps);
DECL void Transfer__Encode_8bit(void **stream, unsigned char n_deps, unsigned int *deps);
DECL void Transfer__Decode_32bit(void **stream, int n_deps);
DECL void Transfer__Decode_16bit(void **stream, int n_deps);
DECL void Transfer__Decode_8bit(void **stream, int n_deps);
DECL char Check_Bit(unsigned int res);

DECL transfer_entry * Transfer__NewEntry(char *name) {
    transfer_entry *new_entry = (transfer_entry *) malloc(sizeof (transfer_entry));
    new_entry->transfer_name = (char *) malloc(sizeof (char) * 100);
    sprintf(new_entry->transfer_name, "%s", name);
    fprintf(stderr, "New *Transfer* = %s\n", name);

    new_entry->uts_ps = __TRANSFER_SIZE_CHUNK;
    new_entry->unformatted_transfer_stream = (unsigned int *) malloc(sizeof (unsigned int) * new_entry->uts_ps);
    new_entry->uts_p = 2;

    new_entry->unformatted_transfer_stream[0] = 0;
    new_entry->unformatted_transfer_stream[1] = 0; /* Reservation stations start at 1 */

    new_entry->transfer_stream = NULL;
    new_entry->ts_p = 0;

    new_entry->task_ids = 0;
    return new_entry;
}

DECL void Transfer__EntryResize(transfer_entry *ent) {
    /* Resize when needed */
    if (ent->uts_p >= ent->uts_ps - 50) {
        unsigned int old_ps = ent->uts_p;
        ent->uts_ps += __TRANSFER_SIZE_CHUNK;
        unsigned int *tmp = (unsigned int *) malloc(sizeof (unsigned int) * ent->uts_ps);
        memcpy(tmp, ent->unformatted_transfer_stream, ent->uts_p * sizeof (unsigned int));
        free(ent->unformatted_transfer_stream);
        ent->unformatted_transfer_stream = tmp;
    }
}

DECL void Transfer__RecordDependency(transfer_entry *ent, unsigned int reservation_nr, unsigned int modifier) {
    Transfer__EntryResize(ent);
    ent->unformatted_transfer_stream[ ent->uts_p++ ] = (reservation_nr << 2) | modifier;
    if (reservation_nr > ent->unformatted_transfer_stream[1])
        ent->unformatted_transfer_stream[1] = reservation_nr;
}

DECL void Transfer__CompressAndManage(transfer_entry *ent) {
    ent->unformatted_transfer_stream[1]++;
    ent->transfer_stream = Transfer__Analyze(&ent->unformatted_transfer_stream[0], &ent->unformatted_transfer_stream[ ent->uts_p ], &ent->req_res_station);
}

DECL void Transfer__RecordTaskHeader(transfer_entry *ent, unsigned int deps) {
    Transfer__EntryResize(ent);
    ent->unformatted_transfer_stream[0]++;
    ent->unformatted_transfer_stream[ent->uts_p++] = ent->task_ids++;
    ent->unformatted_transfer_stream[ent->uts_p++] = deps;
}

DECL char Check_Bit(unsigned int res) {
    if (res < 256) return M_8BIT;
    if (res < 65536) return M_16BIT;
    return M_32BIT;
}

int TESTER[2][65536];
int TESTER_PTR[2] = {0, 0};

DECL void Transfer__Decode_8bit(void **stream, int n_deps) {
    unsigned char *decode = (unsigned char *) *stream;
    int ij;
    for (ij = 0; ij < n_deps; ij++) {
        TESTER[1][ TESTER_PTR[1]++ ] = *decode++;
    }
    *stream = decode;
}

DECL void Transfer__Decode_16bit(void **stream, int n_deps) {
    unsigned short *decode = (unsigned short *) *stream;
    int ij;
    for (ij = 0; ij < n_deps; ij++) {
        TESTER[1][ TESTER_PTR[1]++ ] = *decode++;
    }
    *stream = decode;
}

DECL void Transfer__Decode_32bit(void **stream, int n_deps) {
    fprintf(stderr, "32-bit encodings not supported\n");
    abort();
    unsigned int *decode = (unsigned int *) *stream;
    int ij;
    for (ij = 0; ij < n_deps; ij++) {
        TESTER[1][ TESTER_PTR[1]++ ] = *decode++;
    }

    *stream = decode;
}


//344

DECL void Transfer__Encode_8bit(void **stream, unsigned char n_deps, unsigned int *deps) {
    unsigned char **out_stream = (unsigned char **) stream;
    unsigned char *encode = (unsigned char *) *out_stream;
    *encode++ = (n_deps << 2) | M_8BIT;
    /*fprintf(stderr,"Deps: %d\n",encode[-1] >> 2);*/
    int ij;
    for (ij = 0; ij < n_deps; ij++)
        *encode++ = *deps++;
    *out_stream = encode;
}

DECL void Transfer__Encode_16bit(void **stream, unsigned int n_deps, unsigned int *deps) {
    unsigned short **out_stream = (unsigned short **) stream;
    unsigned char *encode2 = (unsigned char *) *out_stream;
    *encode2++ = (n_deps << 2) | M_16BIT;
    /*fprintf(stderr,"Deps: %d\n",encode2[-1] >> 2);*/
    unsigned short *encode = (unsigned short *) encode2;
    unsigned ij;
    for (ij = 0; ij < n_deps; ij++)
        *encode++ = *deps++;
    *out_stream = encode;
}

DECL void Transfer__Encode_32bit(void **stream, unsigned int n_deps, unsigned int *deps) {
    fprintf(stderr, "32-bit encodings not supported\n");
    abort();
    unsigned int **out_stream = (unsigned int **) stream;
    unsigned int *encode = (unsigned int *) *out_stream;
    *(unsigned char*) encode++ = (n_deps << 2) | M_32BIT;
    unsigned ij;
    for (ij = 0; ij < n_deps; ij++)
        *encode++ = *deps++;
    *out_stream = encode;
}

DECL void* Transfer__Analyze(void *stream, void *end, unsigned int *res) {
    unsigned int *analyze_stream = (unsigned int *) stream;
    unsigned int *analyze_stream_end = (unsigned int *) end;

    typedef struct {
        unsigned int *Order;
        unsigned int OrderNum;
    } Transfer__SimNode;

    Transfer__SimNode *Analysis;

    unsigned int num_task = *analyze_stream++;
    fprintf(stderr, "Tasks: %d\n", num_task);
    fprintf(stderr, "Reservation stations: %d\n", *analyze_stream);

    unsigned int num_res = *analyze_stream++;
    Analysis = (Transfer__SimNode *) malloc(sizeof (Transfer__SimNode) * num_res);
    unsigned ij;
    for (ij = 0; ij < num_res; ij++) {
        Analysis[ij].OrderNum = 0;
        Analysis[ij].Order = (unsigned int *) malloc(sizeof (unsigned int) * 65536);
    }

    /* Parse the stream and enter into reservation stations*/
    int i_dep;
    unsigned all_task;
    for (all_task = 0; all_task < num_task; all_task++) {
        unsigned int tsk_id = *analyze_stream++;
        unsigned int tsk_deps = *analyze_stream++;

        unsigned il;
        for (il = 0; il < tsk_deps; il++) {
            unsigned int dep_val = *analyze_stream++;
            Analysis[ dep_val >> 2].Order[ Analysis[ dep_val >> 2].OrderNum++] = (tsk_id << 2) | (dep_val & 0x3);
            //	   if ( (dep_val & 0x3) != DEP_READ) fprintf(stderr,"hoo\n"); else fprintf(stderr,"%d ",dep_val & 0x3);
        };
    };

    /* Now we have reservations stations.
       Create buffer to hold all useless reservation stations 
       Also identify how many new reservation stations needed*/
    char *res_buffer;
    res_buffer = (char *) malloc(sizeof (char) * num_res);
    memset(res_buffer, 0, num_res);
    unsigned int new_res_station = num_res;

    /* Go through all reservation station looking for only Read-After-Read dependencies; remove them.
       More could be added. */
    unsigned i_res, i_order;
    fprintf(stderr, "%d %d\n", num_res, num_res);
    for (i_res = 0; i_res < num_res; i_res++) {
        char r_only = 1;
        for (i_order = 0; i_order < Analysis[i_res].OrderNum; i_order++)
            if ((Analysis[i_res].Order[i_order] & 0x3) != DEP_READ) {
                r_only = 0;
                break;
            }

        if (r_only) {
            /*fprintf(stderr,"Removed dep: %d\n",i_res);*/ res_buffer[i_res] = 1;
            /*new_res_station--;*/
        }
    }

    /* Identify the new amount of stations needed */
    //fprintf(stderr,"New res stations: %d\n",new_res_station);


    /* Compress */

    fprintf(stderr, "Compressing...\n");
    analyze_stream = (unsigned int *) stream;
    analyze_stream++;
    analyze_stream++;

    unsigned char *out_stream = (unsigned char *) malloc(sizeof (unsigned char) * ((unsigned long) end - (unsigned long) stream));
    unsigned char *out_stream_start = out_stream;
    unsigned short *tmp_stream = (unsigned short *) out_stream;
    *tmp_stream++ = new_res_station;
    out_stream = (unsigned char *) tmp_stream;

    for (all_task = 0; all_task < num_task; all_task++) {
        unsigned int tsk_id = *analyze_stream++; //Ignored 
        unsigned int tsk_deps = *analyze_stream++;

        unsigned il;
        unsigned int Dep_Buffer[tsk_deps];
        unsigned int Dep_Buffer_Num = 0;

        for (il = 0; il < tsk_deps; il++) {
            unsigned int dep = *analyze_stream++;
            //	   TESTER[0][ TESTER_PTR[0]++ ] = dep;
            if (res_buffer[ dep >> 2] == 1)
                continue; /*{fprintf(stderr,"Skipped: %d\n",dep >> 2);continue;}*/
            Dep_Buffer[ Dep_Buffer_Num++] = dep;
        };

        if (Dep_Buffer_Num == 0)
            continue; /*{fprintf(stderr,"Skipped 0 dep task\n");continue;}*/
        /* Select the proper compression */
        unsigned char selected_compression_for_resources = 0;
        unsigned long long _precalc_lock = 0x0;
        for (il = 0; il < Dep_Buffer_Num; il++) {
            _precalc_lock |= ADR(Dep_Buffer[il]);
            if (Check_Bit(Dep_Buffer[il]) > selected_compression_for_resources)
                selected_compression_for_resources = Check_Bit(Dep_Buffer[il]);
        }
        unsigned long long *lock = (unsigned long long *) out_stream;
        *lock = _precalc_lock;
        lock++;

        out_stream = (unsigned char *) lock;
        if (selected_compression_for_resources == M_8BIT) Transfer__Encode_8bit((void **) &out_stream, Dep_Buffer_Num, Dep_Buffer);
        if (selected_compression_for_resources == M_16BIT) Transfer__Encode_16bit((void **) &out_stream, Dep_Buffer_Num, Dep_Buffer);
        if (selected_compression_for_resources == M_32BIT) Transfer__Encode_32bit((void **) &out_stream, Dep_Buffer_Num, Dep_Buffer);
    };

    unsigned short *tmp = (unsigned short *) out_stream_start;
    //   fprintf(stderr,"Final reservation: %d\n",*tmp);
    /*
       for (all_task=0;all_task<num_task;all_task++) {
            unsigned char bitc = *out_stream_start++;

            if ((bitc & 0x3) == M_8BIT) Transfer__Decode_8bit((void**) &out_stream_start,bitc >> 2);
            if ((bitc & 0x3) == M_16BIT) Transfer__Decode_16bit((void**) &out_stream_start,bitc >> 2);
            if ((bitc & 0x3) == M_32BIT) Transfer__Decode_32bit((void**) &out_stream_start,bitc >> 2);

       }     */

    /*
       if (TESTER_PTR[0] != TESTER_PTR[1]) {fprintf(stderr,"Num dep missmatch: %d vs %d\n",TESTER_PTR[0],TESTER_PTR[1]);abort();}
       for (all_task=0;all_task<TESTER_PTR[0]; all_task++)
            { if (TESTER[0][all_task] != TESTER[1][all_task]) {fprintf(stderr,"Missmatch at %d - %d vs %d\n",all_task,TESTER[0][all_task],TESTER[1][all_task]);abort();} }*/
    fprintf(stderr, "Compression complete...\n");
    *res = *tmp;
    for (ij = 0; ij < num_res; ij++)
        free(Analysis[ij].Order);
    free(Analysis);
    free(res_buffer);
    return out_stream_start;
}

#endif

#if defined(SOFTWARE_CACHE)
#define CACHE_SIZE (32768 << 1)
#define CACHE_MASK 0xffff

typedef struct {
    unsigned long adr;
    blysk_bit_node *f_node;
    char valid;
} cache_element;

unsigned long hit = 0, miss = 0;
cache_element CACHE[CACHE_SIZE];

void Add_Cache(unsigned long adr, blysk_bit_node *f_node);

inline blysk_bit_node * Check_Cache(unsigned long adr) {
    unsigned long line = adr & CACHE_MASK;
    if (CACHE[ line ].adr == adr && CACHE[line].valid) {
        hit++;
        return CACHE[ line ]. f_node;
    }
    miss++;
    return NULL;
}

inline void Add_Cache(unsigned long adr, blysk_bit_node *f_node) {
    unsigned long line = adr & CACHE_MASK;
    if (CACHE[ line ] . valid && rand() % 8 == 0) return;
    CACHE[ line ] .adr = adr;
    CACHE[ line ] . valid = 1;
    CACHE[ line ] . f_node = f_node;
}
#endif

