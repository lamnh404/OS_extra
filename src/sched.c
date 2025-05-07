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

static struct queue_t running_list;
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
    unsigned long prio;
    for (prio = 0; prio < MAX_PRIO; prio++)
        if (!empty(&mlq_ready_queue[prio]))
            return -1;
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
    int i;
    for (i = 0; i < MAX_PRIO; i++)
    {
        mlq_ready_queue[i].size = 0;
        slot[i] = MAX_PRIO - i;
    }
#else
    ready_queue.size = 0;
    run_queue.size = 0;
    pthread_mutex_init(&queue_lock, NULL);
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
void put_mlq_proc(struct pcb_t *proc)
{

    enqueue(&mlq_ready_queue[proc->prio], proc);

}

void add_mlq_proc(struct pcb_t *proc)
{

    enqueue(&mlq_ready_queue[proc->prio], proc);

}

struct pcb_t *get_mlq_proc(void)
{
    /*TODO: get a process from PRIORITY [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    struct pcb_t *proc = NULL;
    unsigned long prio;
    bool isEmpty = false;
    curr_prior = 0;
    for (prio = curr_prior; prio < MAX_PRIO; prio++)
    {
        curr_prior = prio;
        if (!empty(&mlq_ready_queue[prio]))
        {
            slot[prio] -= 1;
            proc = dequeue(&mlq_ready_queue[prio]);
            if (slot[prio] == 0 || empty(&mlq_ready_queue[prio]))
            {
                slot[prio] = MAX_PRIO - prio;
                curr_prior++;
                if (curr_prior >= MAX_PRIO)
                {
                    curr_prior = 0;
                }
            }
            break;
        }
        if (curr_prior >= MAX_PRIO)
        {
            curr_prior = 0;
            if (isEmpty)
                break;
            isEmpty = true;
            prio = -1;
        }
    }
    return proc;
}
#endif

// ===== Public Scheduler API =====
struct pcb_t *get_proc(void) {
    struct pcb_t *proc = NULL;

#ifdef CFS_SCHED
    pthread_mutex_lock(&queue_lock);
    pthread_mutex_lock(&cfs_lock);
    proc = cfs_pick_next();
    pthread_mutex_unlock(&cfs_lock);
#elif defined(MLQ_SCHED)
    /*TODO: get a process from [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    return get_mlq_proc();
#else
    struct pcb_t *p = rr_get();
#endif
    pthread_mutex_unlock(&queue_lock);
    return proc;
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
    proc->ready_queue = &ready_queue;
    proc->mlq_ready_queue = mlq_ready_queue;
    proc->running_list = &running_list;

    /* TODO: put running proc to running_list */

    add_mlq_proc(proc);

    enqueue(&ready_queue, proc);

    proc->running_list = &run_queue; //temp

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
    proc->ready_queue = &ready_queue;
    proc->mlq_ready_queue = mlq_ready_queue;
    proc->running_list = &running_list;

    /* TODO: put running proc to running_list */
    put_mlq_proc(proc);
    dequeue(&run_queue);
    enqueue(&run_queue, proc);
    proc->running_list = &run_queue; //temp

#else
    rr_put(proc);
#endif
    pthread_mutex_unlock(&queue_lock);
}
