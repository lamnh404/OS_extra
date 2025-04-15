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
        i = tmp; //how and why
        proc_name[i] = data;
        if(data == -1) proc_name[i] = '\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    // running_list
    if (caller->running_list != NULL) {
        struct queue_t *q = caller->running_list;
        int write_idx = 0;
        for (int read_idx = 0; read_idx < q->size; read_idx++) {
            char temp_name[100];
            BYTE* storage = q->proc[read_idx]->mram->storage;
            int j = 0;
            while (storage[j] != -1 && storage[j] != '\0') {
                temp_name[j] = storage[j];
                j++;
            }
            temp_name[j] = '\0';
            if (q->proc[read_idx] != NULL && strcmp(temp_name, proc_name) == 0) {
                // free_pcb_memph(q->proc[read_idx]); //nothing happen
                q->proc[read_idx]->pc = q->proc[read_idx]->code->size;
                q->proc[read_idx] = NULL;
            } else {
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
            char temp_name[100];
            BYTE* storage = q->proc[read_idx]->mram->storage;
            int j = 0;
            while (storage[j] != -1 && storage[j] != '\0') {
                temp_name[j] = storage[j];
                j++;
            }
            temp_name[j] = '\0';
            if (q->proc[read_idx] != NULL && strcmp(temp_name, proc_name) == 0) {
                // free_pcb_memph(q->proc[read_idx]); (not working)
                free(q->proc[read_idx]); //khong biet co free duoc khong, chua test
                q->proc[read_idx] = NULL;
            } else {
                if (read_idx != write_idx) {
                    q->proc[write_idx] = q->proc[read_idx];
                    q->proc[read_idx] = NULL;
                }
                write_idx++;
            }
        }
        q->size = write_idx;
    }
    return 0; 
}