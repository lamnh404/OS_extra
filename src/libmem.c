/*
* libmem.c - System Library Memory Module Library
*
* Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
*
* Sierra release
* Source Code License Grant: The authors hereby grant Licensee personal
* permission to use and modify the Licensed Source Code for the sole purpose
* of studying while attending the course CO2018.
*/

#include "string.h"
#include "mm.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "../include/syscall.h"
#define DEBUG_PRINT
/* Global mutex to ensure thread-safe modifications of VM structures. */
static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

/**
* enlist_vm_freerg_list - Adds a new free region to the VM area's free list.
* @mm: Pointer to the memory management structure.
* @rg_elmt: The vm_rg_struct element to add.
*
* Returns 0 on success or -1 if the region is invalid.
*/

/**
 * Enlist for merge freerg_list
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
    struct vm_rg_struct **pp = &mm->mmap->vm_freerg_list;
    struct vm_rg_struct  *cur = mm->mmap->vm_freerg_list;

    if (rg_elmt->rg_start >= rg_elmt->rg_end)
        return -1;

    while (cur && cur->rg_start < rg_elmt->rg_start) {
        pp  = &cur->rg_next;
        cur =  cur->rg_next;
    }

    rg_elmt->rg_next = cur;
    *pp               = rg_elmt;

    struct vm_rg_struct *iter = mm->mmap->vm_freerg_list;
    while (iter && iter->rg_next) {
        if (iter->rg_end >= iter->rg_next->rg_start) {
            if (iter->rg_end < iter->rg_next->rg_end) {
                iter->rg_end = iter->rg_next->rg_end;
            }
            struct vm_rg_struct *tmp = iter->rg_next;
            iter->rg_next = tmp->rg_next;
        } else {
            iter = iter->rg_next;
        }
    }

    return 0;
}


/**
* get_symrg_byid - Retrieves a symbol region from the region table.
* @mm: Pointer to the memory management structure.
* @rgid: The symbol region ID.
*
* Returns a pointer to the corresponding vm_rg_struct, or NULL if invalid.
*/
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return NULL;
  return &mm->symrgtbl[rgid];
}

/**
* __alloc - Allocate a memory region within a given VM area.
*
* This function first attempts to obtain a free region of the desired size.
* If no free region is available, it extends the VM area by invoking a syscall.
*
* @caller: Pointer to the calling process's control block.
* @vmaid: VM area identifier.
* @rgid: Region ID to be used in the symbol region table.
* @size: Requested allocation size.
* @alloc_addr: Pointer to store the start address of the allocated region.
*
* Returns 0 on success, or -1 on failure.
*/
int __alloc(struct pcb_t *caller,
            int vmaid,
            int rgid,
            int size,
            int *alloc_addr)
{
    pthread_mutex_lock(&mmvm_lock);

    struct vm_rg_struct rgnode;
    if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0) {
        caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
        caller->mm->symrgtbl[rgid].rg_end   = rgnode.rg_end;
        *alloc_addr                         = rgnode.rg_start;
    //Swap page in
    int start_page = rgnode.rg_start / PAGE_SIZE;
    int end_page = rgnode.rg_end / PAGE_SIZE;
    int pgn, fpn;
    for (int i = start_page; i <= end_page; i++) {
      pg_getpage(caller->mm, i, &fpn, caller);
    }
#ifdef DEBUG_PRINT
        printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
        printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n",
               caller->pid, rgid, rgnode.rg_start, size);
        print_pgtbl(caller, 0, caller->mm->mmap->vm_end);
        printf("================================================================\n");
#endif

        pthread_mutex_unlock(&mmvm_lock);
        return 0;
    }

    pthread_mutex_unlock(&mmvm_lock);
    return -1;
}


/**
* __free - Free an allocated memory region.
*
* The function retrieves the region information using the region ID,
* allocates a new free region node, and adds it back into the free list.
*
* @caller: Pointer to the calling process's control block.
* @vmaid: VM area identifier.
* @rgid: Region ID corresponding to the allocated region.
*
* Returns 0 on success, or -1 on failure.
*/
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return -1;

  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
  if (!rgnode)
    return -1;
  rgnode->rg_start = caller->mm->symrgtbl[rgid].rg_start;
  rgnode->rg_end   = caller->mm->symrgtbl[rgid].rg_end;
  rgnode->rg_next  = NULL;

  pthread_mutex_lock(&mmvm_lock);
  if (enlist_vm_freerg_list(caller->mm, rgnode) != 0) {
    free(rgnode);
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  caller->mm->symrgtbl[rgid].rg_start = 0;
  caller->mm->symrgtbl[rgid].rg_end   = 0;
  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/**
* liballoc - Paging-based allocation wrapper using VM area 0.
*
* @proc: Pointer to the process control block.
* @size: Size of the allocation.
* @reg_index: Symbol region ID to store the allocated region details.
*
* Returns 0 on success, or nonzero on failure.
*/
int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;
  return __alloc(proc, 0, reg_index, size, &addr);
}

/**
* libfree - Paging-based free wrapper using VM area 0.
*
* @proc: Pointer to the process control block.
* @reg_index: Symbol region ID identifying the region to be freed.
*
* Returns 0 on success, or nonzero on failure.
*/
int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  return __free(proc, 0, reg_index);
}

/**
* pg_getpage - Ensure the target page is in RAM, swapping it in if necessary.
*
* If the page is not present, it selects a victim page, swaps the victim's
* content out, and maps in the required page.
*
* @mm: Pointer to the memory management structure.
* @pgn: The virtual page number.
* @fpn: Pointer to store the resulting frame number.
* @caller: Pointer to the calling process's control block.
*
* Returns 0 on success, or -1 on failure.
*/
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte)) {
    int vicpgn, swpfpn, vicfpn, tgtfpn;
    if (find_victim_page(caller->mm, &vicpgn) != 0)
      return -1;

    uint32_t vicpte = mm->pgd[vicpgn];
    vicfpn = PAGING_FPN(vicpte);

    if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) != 0)
      return -1;

    tgtfpn = PAGING_PTE_FPN(pte);

    struct sc_regs regs;
    regs.a1 = SYSMEM_SWP_OP;
    regs.a2 = vicfpn;
    regs.a3 = swpfpn;
    if (__sys_memmap(caller, &regs) != 0)
      return -1;

    regs.a1 = SYSMEM_SWP_OP;
    regs.a2 = tgtfpn;
    regs.a3 = vicfpn;
    if (__sys_memmap(caller, &regs) != 0)
      return -1;

    pte_set_swap(&mm->pgd[vicfpn], 0, swpfpn);
    pte_set_fpn(&mm->pgd[pgn], vicfpn);
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(mm->pgd[pgn]);
  return 0;
}

/**
* pg_getval - Read a byte from a virtual address.
*
* Ensures that the corresponding page is in RAM (or swapped in) before
* performing the I/O read operation via a syscall.
*
* @mm: Pointer to the memory management structure.
* @addr: Virtual address to read from.
* @data: Pointer to store the read byte.
* @caller: Pointer to the process control block.
*
* Returns 0 on success, or -1 on failure.
*/
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn * PAGING_PAGESZ) + off;
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  regs.a3 = 0;
  if (__sys_memmap(caller, &regs) != 0)
    return -1;
  *data = (BYTE)regs.a3;
  return 0;
}

/**
* pg_setval - Write a byte to a virtual address.
*
* Ensures that the page containing the address is available in RAM,
* then writes the data via a syscall.
*
* @mm: Pointer to the memory management structure.
* @addr: Virtual address to write to.
* @value: Byte value to write.
* @caller: Pointer to the process control block.
*
* Returns 0 on success, or -1 on failure.
*/
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn * PAGING_PAGESZ) + off;
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;
  if (__sys_memmap(caller, &regs) != 0)
    return -1;
  return 0;
}

/**
* __read - Internal function to read a byte from a memory region.
*
* Computes the effective physical address by adding the offset to the start
* of the region, then invokes pg_getval.
*
* @caller: Pointer to the process control block.
* @vmaid: VM area identifier.
* @rgid: Symbol region ID.
* @offset: Offset into the region.
* @data: Pointer to store the read value.
*
* Returns 0 on success, or -1 on failure.
*/
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL)
      return -1;

  return pg_getval(caller->mm, currg->rg_start + offset, data, caller);
}

/**
* libread - Public function to read a byte from a region memory.
*
* Reads from a given memory region (assumed on VM area 0) and, if successful,
* writes the byte value into the destination.
*
* @proc: Pointer to the process control block.
* @source: Symbol region ID representing the region.
* @offset: Offset into the region.
* @destination: Pointer to a uint32_t variable where the read value is stored.
*
* Returns 0 on success, or nonzero on failure.
*/
// int libread(struct pcb_t *proc, uint32_t source, uint32_t offset, uint32_t *destination)
// {
//   BYTE data;
//   int val = __read(proc, 0, source, offset, &data);
//   if (val == 0)
//     *destination = (uint32_t)data;

// #ifdef IODUMP
//   printf("read region=%d offset=%d value=%d\n", source, offset, data);
// #ifdef PAGETBL_DUMP
//   print_pgtbl(proc, 0, -1);
// #endif
//   MEMPHY_dump(proc->mram);
// #endif
//   return val;
// }
int libread(struct pcb_t *proc, uint32_t source, uint32_t offset, uint32_t *destination)
{
    BYTE data;
    int val = __read(proc, 0, source, offset, &data);
    if (val == 0)
        *destination = (uint32_t)data;

#ifdef DEBUG_PRINT
    printf("===== PHYSICAL MEMORY AFTER READING =====\n");
    printf("read region=%d offset=%d value=%d\n", source, offset, data);
    print_pgtbl(proc, 0, proc->mm->mmap->vm_end);
    MEMPHY_dump(proc->mram);
    printf("================================================================\n");
#endif
    return val;
}

/**
* __write - Internal function to write a byte to a memory region.
*
* @caller: Pointer to the process control block.
* @vmaid: VM area identifier.
* @rgid: Symbol region ID.
* @offset: Offset within the region.
* @value: Byte value to write.
*
* Returns 0 on success, or -1 on failure.
*/
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL)
    return -1;

  return pg_setval(caller->mm, currg->rg_start + offset, value, caller);
}

/**
* libwrite - Public function to write a byte to a region memory.
*
* @proc: Pointer to the process control block.
* @data: Byte value to write.
* @destination: Symbol region ID representing the region.
* @offset: Offset within the region.
*
* Returns 0 on success, or nonzero on failure.
*/
// int libwrite(struct pcb_t *proc, BYTE data, uint32_t destination, uint32_t offset)
// {
// #ifdef IODUMP
//   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
// #ifdef PAGETBL_DUMP
//   print_pgtbl(proc, 0, -1);
// #endif
//   MEMPHY_dump(proc->mram);
// #endif
//   return __write(proc, 0, destination, offset, data);
// }
int libwrite(struct pcb_t *proc, BYTE data, uint32_t destination, uint32_t offset)
{
#ifdef DEBUG_PRINT
    printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
#endif

    int val = __write(proc, 0, destination, offset, data);

#ifdef DEBUG_PRINT
    printf("write region=%d offset=%d value=%d\n", destination, offset, data);
    print_pgtbl(proc, 0, proc->mm->mmap->vm_end);
    MEMPHY_dump(proc->mram);
    printf("================================================================\n");
#endif

    return val;
}

/**
* free_pcb_memph - Frees all physical frames allocated to a process.
*
* Iterates over the page directory and returns frames to the free pool.
*
* @caller: Pointer to the process control block.
*
* Returns 0 on success.
*/
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++) {
    pte = caller->mm->pgd[pagenum];
    if (!PAGING_PAGE_PRESENT(pte)) {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

  return 0;
}

/**
* find_victim_page - Implements a FIFO policy to select a victim page.
*
* @mm: Pointer to the memory management structure.
* @retpgn: Pointer to store the selected victim page number.
*
* Returns 0 on success, or -1 if no victim page is available.
*/
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;
  if (pg == NULL)
    return -1;
  
  *retpgn = pg->pgn;
  mm->fifo_pgn = pg->pg_next;
  free(pg);
  
  return 0;
}

/**
* get_free_vmrg_area - Searches for an adequately sized free region.
*
* Iterates over the free region list of the given VM area until a region of
* at least 'size' bytes is found. Updates the free list appropriately.
*
* @caller: Pointer to the process control block.
* @vmaid: VM area identifier.
* @size: Required allocation size.
* @newrg: Pointer to a vm_rg_struct to populate with the allocated region.
*
* Returns 0 if a suitable region is found, or -1 otherwise.
*/
// int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
// {
//   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
//   if (cur_vma == NULL)
//     return -1;

//   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
//   if (rgit == NULL)
//     return -1;

//   newrg->rg_start = newrg->rg_end = -1;
//   while (rgit != NULL) {
//     int rg_size = rgit->rg_end - rgit->rg_start;
//     if (rg_size >= size) {
//       newrg->rg_start = rgit->rg_start;
//       newrg->rg_end   = rgit->rg_start + size;

//       if (rg_size > size) {
//         rgit->rg_start += size;
//       } else {
//         cur_vma->vm_freerg_list = rgit->rg_next;
//         free(rgit);
//       }
//       return 0;
//     }
//     rgit = rgit->rg_next;
//   }

//   return -1;
// }

int get_free_vmrg_area(struct pcb_t *caller,
                       int vmaid,
                       int size,
                       struct vm_rg_struct *newrg)
{
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
    if (!cur_vma)
        return -1;

    int expanded = 0;
    int  old_end  = cur_vma->vm_end;

retry_search:
    struct vm_rg_struct **best_prev = NULL;
    struct vm_rg_struct  *best      = NULL;
    int                   best_diff = INT32_MAX;
    int                   tail_free = 0;

    struct vm_rg_struct **pp  = &cur_vma->vm_freerg_list;
    struct vm_rg_struct  *cur = cur_vma->vm_freerg_list;
    while (cur) {
        int region_size = cur->rg_end - cur->rg_start;
        int diff = region_size - size;

        if (diff >= 0 && diff < best_diff) {
            best_diff = diff;
            best      = cur;
            best_prev = pp;
            if (diff == 0)
                break; 
        }

        if (!expanded && cur->rg_end == old_end) {
            tail_free = region_size;
        }

        pp  = &cur->rg_next;
        cur =  cur->rg_next;
    }

    if (best) {
        newrg->rg_start = best->rg_start;
        newrg->rg_end   = best->rg_start + size;

        if (best_diff > 0) {
            /* shrink front of the free region */
            best->rg_start += size;
        } else {
            /* exact fit: remove the node */
            *best_prev = best->rg_next;
            free(best);
        }
        return 0;
    }

    /* No best‐fit yet: expand once, then retry */
    if (!expanded) {
        int needed = size - tail_free;

        struct sc_regs regs;
        regs.a1 = SYSMEM_INC_OP;
        regs.a2 = vmaid;
        regs.a3 = needed;

        if (__sys_memmap(caller, &regs) != 0) {
          pthread_mutex_unlock(&mmvm_lock);
          return -1;
        }

        struct vm_rg_struct *added = malloc(sizeof(*added));
        if (!added)
            return -1;

        added->rg_start = old_end;
        added->rg_end   = cur_vma->vm_end;
        added->rg_next  = NULL;
        enlist_vm_freerg_list(caller->mm, added);

        expanded = 1;
        goto retry_search;
    }

    return -1;
}