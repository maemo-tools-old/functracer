#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "debug.h"
#include "syscall.h"
#include "target_mem.h"

#define off_r0 0
#define off_r7 28
#define off_ip 48
#define off_pc 60

int get_syscall_nr(struct process *proc, int *nr)
{
	/* get the user's pc (plus 8) */
	int pc = trace_user_readw(proc, off_pc);
	/* fetch the SWI instruction */
	int insn = trace_mem_readw(proc, (addr_t)pc - 4);
	int ip = trace_user_readw(proc, off_ip);

	if (insn == 0xef000000 || insn == 0x0f000000) {
		/* EABI syscall */
		*nr = trace_user_readw(proc, off_r7);
	} else if ((insn & 0xfff00000) == 0xef900000) {
		/* old ABI syscall */
		*nr = insn & 0xfffff;
	} else {
		/* TODO: handle swi<cond> variations */
		/* one possible reason for getting in here is that we
		 * are coming from a signal handler, so the current
		 * PC does not point to the instruction just after the
		 * "swi" one. */
		msg_err("unexpected instruction 0x%x at 0x%x", insn, pc - 4);
		return -1;
	}
	if ((*nr & 0xf0000) == 0xf0000) {
		/* arch-specific syscall */
		*nr &= ~0xf0000;
	}
	/* ARM syscall convention: on syscall entry, ip is zero;
	 * on syscall exit, ip is non-zero */
	return ip ? 2 : 1;
}

long get_syscall_arg(struct process *proc, int arg_num)
{
	if (arg_num == -1)	/* return value */
		return trace_user_readw(proc, off_r0);

	return trace_user_readw(proc, 4 * arg_num);
}
