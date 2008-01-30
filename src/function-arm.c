#include <stdio.h>
#include <stdlib.h>
#include <libiberty.h>

#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <asm/ptrace.h>

#include "debug.h"
#include "function.h"
#include "target_mem.h"

static int callstack_depth = 0; /* XXX debug */

#define off_lr 56
#define off_sp 52
#define off_r0 0

static addr_t get_stack_pointer(struct process *proc)
{
	return trace_user_readw(proc, off_sp);
}

#define long2bytes(w)	(unsigned char *)&w

long fn_argument(struct process *proc, int arg_num)
{
	long w;
	addr_t sp;

	debug(3, "fn_argument(pid=%d, arg_num=%d)", proc->pid, arg_num);
	if (proc->callstack == NULL) {
		debug(1, "no function argument data is saved");
		if (arg_num < 4)
			w = trace_user_readw(proc, 4 * arg_num);
		else {
			sp = get_stack_pointer(proc);
			w = trace_mem_readw(proc, sp + 4 * (arg_num - 4));
		}
	} else {
		struct pt_regs *regs;
		regs = (struct pt_regs *)proc->callstack->fn_arg_data;
		if (arg_num < 4)
			w = regs->uregs[arg_num];
		else {
			sp = regs->ARM_sp;
			w = trace_mem_readw(proc, sp + 4 * (arg_num - 4));
		}
	}

	return w;
}

long fn_return_value(struct process *proc)
{
	debug(3, "fn_return_value(pid=%d)", proc->pid);
	return trace_user_readw(proc, off_r0);
}

void fn_return_address(struct process *proc, addr_t *addr)
{
	debug(3, "fn_return_address(pid=%d)", proc->pid);
	*addr = trace_user_readw(proc, off_lr);
}

void fn_save_arg_data(struct process *proc)
{
	struct callstack *cs;

	debug(1, "depth = %d", ++callstack_depth);

	cs = xmalloc(sizeof(struct callstack));
	cs->fn_arg_data = xmalloc(sizeof(struct pt_regs));
	trace_getregs(proc, cs->fn_arg_data);
	cs->next = proc->callstack;
	proc->callstack = cs;
}

void fn_invalidate_arg_data(struct process *proc)
{
	struct callstack *cs;

	debug(1, "depth = %d", callstack_depth--);

	if (proc->callstack == NULL) {
		debug(1, "trying to invalidade non-existent arg data");
		return;
	}
	cs = proc->callstack->next;
	free(proc->callstack->fn_arg_data);
	free(proc->callstack);
	proc->callstack = cs;
}