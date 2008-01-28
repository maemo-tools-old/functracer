#include <linux/ptrace.h>

#include "syscall.h"
#include "target_mem.h"

int get_syscall_nr(struct process *proc, int *nr)
{
	*nr = trace_user_readw(proc, 4 * ORIG_EAX);
	if (proc->in_syscall) {
		proc->in_syscall = 0;
		return 2;
	}
	if (*nr >= 0) {
		proc->in_syscall = 1;
		return 1;
	}
	return 0;
}

long get_syscall_arg(struct process *proc, int arg_num)
{
	if (arg_num == -1)	/* return value */
		return trace_user_readw(proc, 4 * EAX);

	return trace_user_readw(proc, 4 * arg_num);
}
