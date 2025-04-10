// #ifdef MM_PAGING
/**
 * @file mm-vm.c
 * @brief Paging-based Virtual Memory Management.
 *
 * This module implements virtual memory management functions for a paging
 * system. It includes functions to locate and extend VM areas and to swap pages.
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/**
 * get_vma_by_num - Retrieve a vm_area_struct by its numeric ID.
 * @mm: Pointer to the memory management structure.
 * @vmaid: The requested VM area ID.
 *
 * Returns a pointer to the VM area if found, or NULL otherwise.
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;
  if (pvma == NULL)
    return NULL;

  /* Traverse until we find an area whose vm_id is >= vmaid */
  while (pvma != NULL && pvma->vm_id < vmaid) {
    pvma = pvma->vm_next;
  }
  return pvma;
}

/**
 * get_vm_area_node_at_brk - Allocate a new VM region node at the current break.
 * @caller: Pointer to the calling process's control block.
 * @vmaid: The VM area ID where memory will be allocated.
 * @size: Requested allocation size (in bytes).
 * @alignedsz: The allocation size after page alignment.
 *
 * Returns a pointer to a freshly allocated vm_rg_struct or NULL on failure.
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (cur_vma == NULL)
    return NULL;
  
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL)
    return NULL;
  
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end   = cur_vma->sbrk + alignedsz;
  newrg->rg_next  = NULL;
  return newrg;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn , int swpfpn)
{
  __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
  return 0;
}

/**
 * validate_overlap_vm_area - Check that a memory region does not overlap.
 * @caller: Pointer to the calling process's control block.
 * @vmaid: The target VM area ID.
 * @vmastart: Proposed start address.
 * @vmaend: Proposed end address.
 *
 * Returns 0 if there is no overlap; otherwise, -1.
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct vm_area_struct *vma = caller->mm->mmap;
  while (vma != NULL) {
    if (vma->vm_id != vmaid) {
      if (!(vmaend <= vma->vm_start || vmastart >= vma->vm_end))
      return -1; /* Overlap detected */
    }
    vma = vma->vm_next;
  }
  return 0;
}

/**
 * inc_vma_limit - Extend the VM area limit.
 * @caller: Pointer to the calling process's control block.
 * @vmaid: VM area ID to extend.
 * @inc_sz: Size to increase by (in bytes).
 *
 * Returns 0 on success, or -1 on failure.
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
    struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
    if (!newrg)
      return -1;
    
    int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
    int incnumpage = inc_amt / PAGING_PAGESZ;
    struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
    if (area == NULL) {
      free(newrg);
      return -1;
    }
    
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
    if (cur_vma == NULL) {
      free(newrg);
      free(area);
      return -1;
    }
    
    int old_end = cur_vma->vm_end;
    if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
      free(newrg);
      free(area);
      return -1;
    }
    
    cur_vma->vm_end = area->rg_end;
    cur_vma->sbrk  = area->rg_end;
    
    if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage, newrg) < 0) {
      free(newrg);
      free(area);
      return -1;
    }
    
    free(area);
    return 0;
}
// int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
// {
//   int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
//   int incnumpage = inc_amt / PAGING_PAGESZ;
//   struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
//   if (area == NULL) {
//     return -1;
//   }
  
//   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
//   if (cur_vma == NULL) {
//     free(area);
//     return -1;
//   }
  
//   int old_end = cur_vma->vm_end;
//   if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
//     free(area);
//     return -1;
//   }
  
//   cur_vma->vm_end = area->rg_end;
//   cur_vma->sbrk  = area->rg_end;
  
//   // Pass 'area' to vm_map_ram instead of an uninitialized 'newrg'
//   if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage, area) < 0) {
//     free(area);
//     return -1;
//   }
  
//   return 0;
// }

// #endif  // MM_PAGING