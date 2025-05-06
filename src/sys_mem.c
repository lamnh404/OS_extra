/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

 #include "syscall.h"
 #include "libmem.h"
 #include "mm.h"
 #include <stdio.h>
 
 int __sys_memmap(struct pcb_t *caller, struct sc_regs* regs)
 {
     int memop = regs->a1;
     BYTE value;
     int ret = 0;
 
     switch (memop) {
     case SYSMEM_MAP_OP:
         /* Reserved process mapping, if needed */
         break;
     case SYSMEM_INC_OP:
         ret = inc_vma_limit(caller, regs->a2, regs->a3);
         if (ret != 0)
             fprintf(stderr, "SYSMEM_INC_OP failed\n");
         break;
     case SYSMEM_SWP_OP:
         ret = __mm_swap_page(caller, regs->a2, regs->a3);
         if (ret != 0)
             fprintf(stderr, "SYSMEM_SWP_OP failed\n");
         break;
     case SYSMEM_IO_READ:
         if (MEMPHY_read(caller->mram, regs->a2, &value) != 0)
             ret = -1;
         else
             regs->a3 = value;
         break;
     case SYSMEM_IO_WRITE:
         ret = MEMPHY_write(caller->mram, regs->a2, regs->a3);
         break;
     default:
         printf("Unknown Memop code: %d\n", memop);
     }
     return ret;
 }
 