#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "syscall.h"

#if __i386__

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

#else

#define off_r0 0
#define off_ip 48
#define off_pc 60

int get_syscall_nr(struct process *proc, int *nr)
{
	/* get the user's pc (plus 8) */
	int pc = ptrace(PTRACE_PEEKUSER, proc->pid, off_pc, 0);
	/* fetch the SWI instruction */
	int insn = ptrace(PTRACE_PEEKTEXT, proc->pid, pc - 4, 0);

	*nr = insn & 0xFFFF;
	/* if it is a syscall, return 1 or 2 */
	if ((insn & 0xFFFF0000) == 0xef900000) {
		return ptrace(PTRACE_PEEKUSER, proc->pid, off_ip, 0) ? 2 : 1;
	} else if ((insn & 0xFFFF0000) == 0xef9f0000) {
		/* nr == 1 is an old-style breakpoint, but the PC is off */
		return ptrace(PTRACE_PEEKUSER, proc->pid, off_ip, 0) ? 4 : 3;
	} else if ((insn & 0xFFFF0000) == 0xef000000) {
		struct pt_regs regs;
		if (ptrace(PTRACE_GETREGS, proc->pid, 0, &regs) != -1) {
			*nr = regs.ARM_r7;
			return ptrace(PTRACE_PEEKUSER, proc->pid, off_ip, 0) ? 2 : 1;
		}
	}
	return 0;
}

long get_syscall_arg(struct process *proc, int arg_num)
{
	if (arg_num == -1)	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, off_r0, 0);

	return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * arg_num, 0);
}

#endif
