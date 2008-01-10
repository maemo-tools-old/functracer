#include <stdio.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <errno.h>

#include "sysdeps.h"
#include "process.h"
#include "target_mem.h"

int get_instruction_pointer(struct process *proc, void **addr)
{
	long w;

	trace_user_read(proc, 4 * EIP, &w);
	*addr = (void *)w;

	return 0;
}

int set_instruction_pointer(struct process *proc, void *addr)
{
	trace_user_write(proc, 4 * EIP, (long)addr);

	return 0;
}
