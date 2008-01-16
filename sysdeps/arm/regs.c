#include <stdio.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <errno.h>

#include "sysdeps.h"
#include "process.h"
#include "target_mem.h"

#define off_pc 60

int get_instruction_pointer(struct process *proc, addr_t *addr)
{
	*addr = trace_user_readw(proc, off_pc);

	return 0;
}

int set_instruction_pointer(struct process *proc, addr_t addr)
{
	trace_user_writew(proc, off_pc, (long)addr);

	return 0;
}
