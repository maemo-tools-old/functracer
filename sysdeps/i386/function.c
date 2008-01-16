#include <stdio.h>
#include <stdlib.h>
#include <libiberty.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "debug.h"
#include "function.h"

static int callstack_depth = 0; /* XXX debug */

static addr_t get_stack_pointer(struct process *proc)
{
	return trace_user_readw(proc, 4 * UESP);
}

long fn_argument(struct process *proc, int arg_num)
{
	addr_t sp;

	debug(3, "fn_argument(pid=%d, arg_num=%d)", proc->pid, arg_num);
	if (proc->callstack == NULL) {
		debug(1, "no function argument data is saved");
		sp = get_stack_pointer(proc);
	} else {
		sp = (addr_t)proc->callstack->fn_arg_data;
	}
	return trace_mem_readw(proc, sp + 4 * (arg_num + 1));
}

long fn_return_value(struct process *proc)
{
	debug(3, "fn_argument(pid=%d)", proc->pid);

	return trace_user_readw(proc, 4 * EAX);
}

void fn_return_address(struct process *proc, addr_t *addr)
{
	addr_t sp;

	debug(3, "fn_return_address(pid=%d)", proc->pid);
	sp = get_stack_pointer(proc);
	*addr = trace_mem_readw(proc, sp);
}

void fn_save_arg_data(struct process *proc)
{
	struct callstack *cs;

	debug(1, "depth = %d", ++callstack_depth);
	cs = xmalloc(sizeof(struct callstack));
	cs->fn_arg_data = (void *)get_stack_pointer(proc);
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
	free(proc->callstack);
	proc->callstack = cs;
}
