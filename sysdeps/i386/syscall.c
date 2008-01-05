#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "syscall.h"

int get_syscall_nr(struct process *proc, int *nr)
{
	*nr = ptrace(PTRACE_PEEKUSER, proc->pid, 4 * ORIG_EAX, 0);
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
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * EAX, 0);

	return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * arg_num, 0);
}
