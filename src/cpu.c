#include "cpu.h"
#include "mem.h"
#include "mm.h"
#include "syscall.h"
#include "libmem.h"

/**
 * calc - Dummy calculation function.
 * @proc: Pointer to the process control block.
 *
 * Returns a computed value. (Here, as an example, we simply return the content of register 0.)
 */
int calc(struct pcb_t *proc)
{
        return ((unsigned long)proc & 0UL);
}

/**
 * alloc - Allocate memory for a process.
 * @proc: Pointer to the process control block.
 * @size: Requested memory size.
 * @reg_index: Register index where the allocated address will be stored.
 *
 * Returns 0 on success, nonzero on error.
 */
int alloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
addr_t addr = alloc_mem(size, proc);
if (addr == 0)
    return 1;
proc->regs[reg_index] = addr;
return 0;
}

/**
 * free_data - Free memory allocated to a process.
 * @proc: Pointer to the process control block.
 * @reg_index: Register index holding the address to free.
 *
 * Returns 0 on success.
 */
int free_data(struct pcb_t *proc, uint32_t reg_index)
{
return free_mem(proc->regs[reg_index], proc);
}

/**
 * read - Read a byte from memory.
 * @proc: Pointer to the process control block.
 * @source: Register index holding the base address.
 * @offset: Offset from the base address.
 * @destination: Register index where the read value will be stored.
 *
 * Returns 0 on success, nonzero on error.
 */
int read(struct pcb_t *proc, uint32_t source, uint32_t offset, uint32_t destination)
{
BYTE data;
/* read_mem returns 0 on success, so check for nonzero error code */
if (read_mem(proc->regs[source] + offset, proc, &data) == 0) {
    proc->regs[destination] = data;
    return 0;
}
return 1;
}

/**
 * write - Write a byte to memory.
 * @proc: Pointer to the process control block.
 * @data: Byte value to write.
 * @destination: Register index holding the base address.
 * @offset: Offset from the base address.
 *
 * Returns 0 on success, nonzero on error.
 */
int write(struct pcb_t *proc, BYTE data, uint32_t destination, uint32_t offset)
{
return write_mem(proc->regs[destination] + offset, proc, data);
}

/**
 * run - Execute one instruction from the process code.
 * @proc: Pointer to the process control block.
 *
 * Returns 0 if instruction executed successfully, 1 on failure or if process finished.
 */
int run(struct pcb_t *proc)
{
if (proc->pc >= proc->code->size)
    return 1;   /* Process finished */

struct inst_t ins = proc->code->text[proc->pc];
proc->pc++;
int stat = 1;

switch (ins.opcode)
{
    case CALC:
        stat = calc(proc);
        break;
    case ALLOC:
#ifdef MM_PAGING
        stat = liballoc(proc, ins.arg_0, ins.arg_1);
#else
        stat = alloc(proc, ins.arg_0, ins.arg_1);
#endif
        break;
    case FREE:
#ifdef MM_PAGING
        stat = libfree(proc, ins.arg_0);
#else
        stat = free_data(proc, ins.arg_0);
#endif
        break;
    case READ:
#ifdef MM_PAGING
        stat = libread(proc, ins.arg_0, ins.arg_1, &ins.arg_2);
#else
        stat = read(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#endif
        break;
    case WRITE:
#ifdef MM_PAGING
        stat = libwrite(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#else
        stat = write(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#endif
        break;
    case SYSCALL:
        stat = libsyscall(proc, ins.arg_0, ins.arg_1, ins.arg_2, ins.arg_3);
        break;
    default:
        stat = 1;
}
return stat;
}
