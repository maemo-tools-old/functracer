#include <stdio.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <errno.h>

#include "sysdeps.h"
#include "process.h"

int get_instruction_pointer(struct process *proc, void **addr)
{
	long w;

	errno = 0;
	w = ptrace(PTRACE_PEEKUSER, proc->pid, 4 * EIP, 0);
	if (w == -1 && errno)
		return -1;
	*addr = (void *)w;

	return 0;
}

int set_instruction_pointer(struct process *proc, void *addr)
{
	return 0;
}
