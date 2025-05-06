#include "queue.h"
#include "sched.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef CFS_SCHED
#include "cfs.h"
#endif

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
static int curr_prior = 0;
#endif

#ifdef CFS_SCHED
static pthread_mutex_t cfs_lock; // protect CFS runqueue
#endif

// ===== Generic Helpers =====
int queue_empty(void) {
#ifdef CFS_SCHED
    return (cfs_rq.total_weight == 0) ? 0 : -1;
#elif defined(MLQ_SCHED)
    for (int i = 0; i < MAX_PRIO; i++) {
        if (!empty(&mlq_ready_queue[i])) return -1;
    }
    return (empty(&ready_queue) && empty(&run_queue));
#else
    return (empty(&ready_queue) && empty(&run_queue));
#endif
}

void init_scheduler(void) {
    pthread_mutex_init(&queue_lock, NULL);
#ifdef CFS_SCHED
    pthread_mutex_init(&cfs_lock, NULL);
    cfs_init_rq();
#elif defined(MLQ_SCHED)
    for (int i = 0; i < MAX_PRIO; i++) {
        mlq_ready_queue[i].size = 0;
        slot[i] = MAX_PRIO - i;
    }
#else
    ready_queue.size = 0;
    run_queue.size = 0;
#endif
}

// ===== Round-Robin Implementation =====
static void rr_refill(void) {
    while (!empty(&run_queue)) {
        enqueue(&ready_queue, dequeue(&run_queue));
    }
}

static struct pcb_t *rr_get(void) {
    if (empty(&ready_queue) && !empty(&run_queue)) {
        rr_refill();
    }
    return empty(&ready_queue) ? NULL : dequeue(&ready_queue);
}

static void rr_add(struct pcb_t *proc) {
    enqueue(&ready_queue, proc);
}

static void rr_put(struct pcb_t *proc) {
    enqueue(&run_queue, proc);
}

// ===== MLQ Implementation =====
#ifdef MLQ_SCHED
static void add_mlq_proc(struct pcb_t *proc) {
    enqueue(&mlq_ready_queue[proc->prio], proc);
}

static struct pcb_t *get_mlq_proc(void) {
    struct pcb_t *proc = NULL;
    for (int cnt = 0; cnt < MAX_PRIO; cnt++) {
        int prio = (curr_prior + cnt) % MAX_PRIO;
        if (!empty(&mlq_ready_queue[prio])) {
            slot[prio]--;
            proc = dequeue(&mlq_ready_queue[prio]);
            if (slot[prio] == 0 || empty(&mlq_ready_queue[prio])) {
                slot[prio] = MAX_PRIO - prio;
                curr_prior = (prio + 1) % MAX_PRIO;
            } else {
                curr_prior = prio;
            }
            break;
        }
    }
    return proc;
}
#endif

// ===== Public Scheduler API =====
struct pcb_t *get_proc(void) {
    pthread_mutex_lock(&queue_lock);
#ifdef CFS_SCHED
    pthread_mutex_lock(&cfs_lock);
    struct pcb_t *p = cfs_pick_next();
    pthread_mutex_unlock(&cfs_lock);
#elif defined(MLQ_SCHED)
    struct pcb_t *p = mlq_get();
    if (!p) p = rr_get();
#else
    struct pcb_t *p = rr_get();
#endif
    pthread_mutex_unlock(&queue_lock);
    return p;
}

void add_proc(struct pcb_t *proc) {
    pthread_mutex_lock(&queue_lock);
#ifdef CFS_SCHED
    proc->cfs_ent.vruntime = 0;
    proc->cfs_ent.weight   = cfs_compute_weight(proc->cfs_ent.weight);
    pthread_mutex_lock(&cfs_lock);
    cfs_enqueue(proc);
    pthread_mutex_unlock(&cfs_lock);
#elif defined(MLQ_SCHED)
    mlq_add(proc);
#else
    rr_add(proc);
#endif
    pthread_mutex_unlock(&queue_lock);
}

void put_proc(struct pcb_t *proc) {
    pthread_mutex_lock(&queue_lock);
#ifdef CFS_SCHED
    pthread_mutex_lock(&cfs_lock);
    cfs_enqueue(proc);
    pthread_mutex_unlock(&cfs_lock);
#elif defined(MLQ_SCHED)
    mlq_add(proc);
#else
    rr_put(proc);
#endif
    pthread_mutex_unlock(&queue_lock);
}
