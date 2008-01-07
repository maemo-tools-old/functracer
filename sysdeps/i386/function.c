#include <stdio.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "debug.h"
#include "function.h"
#include "ptrace.h"

static void get_stack_pointer(struct process *proc, void **addr)
{
	long sp;

	trace_user_read(proc, 4 * UESP, &sp);
	*addr = (void *)sp;
}

long fn_argument(struct process *proc, int arg_num)
{
	long w;
	unsigned char *w_bytes = (unsigned char *)&w;

	debug(3, "fn_argument(pid=%d, arg_num=%d)", proc->pid, arg_num);
	if (proc->fn_arg_data == NULL)
		fn_save_arg_data(proc);
	trace_mem_read(proc, proc->fn_arg_data + 4 * (arg_num + 1), w_bytes, sizeof(long));

	return w;
}

long fn_return_value(struct process *proc)
{
	long w;

	debug(3, "fn_argument(pid=%d)", proc->pid);
	trace_user_read(proc, 4 * EAX, &w);
	return w;
}

void fn_return_address(struct process *proc, void **addr)
{
	long w;
	unsigned char *w_bytes = (unsigned char *)&w;
	void *sp;

	debug(3, "fn_return_address(pid=%d)", proc->pid);
	get_stack_pointer(proc, &sp);
	trace_mem_read(proc, sp, w_bytes, sizeof(long));
	*addr = (void *)w;
}

void fn_save_arg_data(struct process *proc)
{
	get_stack_pointer(proc, &proc->fn_arg_data);
}

void fn_invalidate_arg_data(struct process *proc)
{
	proc->fn_arg_data = NULL;
}
