#ifndef CFS_H
#define CFS_H

#include <stdint.h>
#include <pthread.h>
#include "rbtree.h"
#include "common.h"
#include "timer.h"    

#ifndef CFS_SCHED
#define CFS_SCHED
#endif
#define SCHED_LATENCY_NSEC   20000000ULL
#define MIN_GRANULARITY_NSEC 1000000ULL
#define WEIGHT_NORM          1024ULL

struct cfs_rq {
    RBTree          *tree;
    uint64_t         total_weight;
    pthread_mutex_t  rq_lock;
};

extern struct cfs_rq cfs_rq;


void     cfs_init_rq(void);
uint32_t cfs_compute_weight(int nice);
void     cfs_enqueue(struct pcb_t *p);
void     cfs_dequeue(struct pcb_t *p);
struct pcb_t *cfs_pick_next(void);
uint64_t cfs_timeslice(struct pcb_t *p);
void     cfs_update_vruntime(struct pcb_t *p, uint64_t delta_ns);
void     cfs_task_tick(struct pcb_t *p, uint64_t elapsed_ns);



#endif /* CFS_H */