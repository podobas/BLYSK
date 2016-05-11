#include <stdio.h>
#include <stdlib.h>




typedef struct {
  unsigned long task;
  unsigned long lineage;
  unsigned long parent;
  unsigned long joins_at;
  unsigned int cpu_id;
  unsigned int child_number;
  unsigned long num_children;
  unsigned long exec_cycles;
  unsigned long creation_cycles;
  unsigned long overhead_cycles;
  unsigned long queue_size;
  unsigned long create_instant;
  unsigned long exec_end_instant;
  char *tag;
  char *metadata;
  char *outline_function;
  char *wait_instants;

  unsigned int cpu_release;
  unsigned long release_instant;
  unsigned long dependency_resolution_time;

} __GG_task_stats;
