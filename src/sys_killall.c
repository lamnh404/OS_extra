/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h"
#include "stdlib.h"
#include "string.h"

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    printf("sys_killall syscall\n");
    char proc_name[100];
    BYTE data;

    //hardcode for demo only
    uint32_t memrg = regs->a3;
    /* TODO: Get name of the target proc */
    int i = 0;
    data = 0;
    while(data != -1){
        int tmp = i;
        libread(caller, memrg, i, &data);
        i = tmp;
        proc_name[i] = (char)data;
        if(data == -1) proc_name[i] = '\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);
    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    int terminated = 0;
    //caller->mlq_ready_queu

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */
    if (caller->running_list != NULL) {
        struct queue_t *q = caller->running_list;
        int write_idx = 0;
        for (int read_idx = 0; read_idx < q->size; read_idx++) {
            // if (q->proc[read_idx] != NULL && strcmp(q->proc[read_idx]->path, proc_name) == 0) {
            if (1) {
                // Terminate the process
                free_pcb_memph(q->proc[read_idx]);
                free(q->proc[read_idx]);
                q->proc[read_idx] = NULL;
                terminated++;
            } else {
                // Keep the process in the queue
                if (read_idx != write_idx) {
                    q->proc[write_idx] = q->proc[read_idx];
                    q->proc[read_idx] = NULL;
                }
                write_idx++;
            }
        }
        q->size = write_idx;
    }

    if (caller->mlq_ready_queue != NULL) {
        struct queue_t *q = caller->mlq_ready_queue;
        int write_idx = 0;
        for (int read_idx = 0; read_idx < q->size; read_idx++) {
            if (q->proc[read_idx] != NULL && strcmp(q->proc[read_idx]->path, proc_name) == 0) {
                // Terminate the process
                free_pcb_memph(q->proc[read_idx]);
                free(q->proc[read_idx]);
                q->proc[read_idx] = NULL;
                terminated++;
            } else {
                // Keep the process in the queue
                if (read_idx != write_idx) {
                    q->proc[write_idx] = q->proc[read_idx];
                    q->proc[read_idx] = NULL;
                }
                write_idx++;
            }
        }
        q->size = write_idx;
    }
    printf("Terminated %d processes with name \"%s\"\n", terminated, proc_name);
    return 0; 
}