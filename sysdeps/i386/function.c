#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "debug.h"
#include "function.h"
#include "ptrace.h"

long get_function_arg(struct process *proc, int arg_num)
{
	long w, sp;
	unsigned char *w_bytes = (unsigned char *)&w;

	debug(3, "get_function_arg(pid=%d, arg_num=%d)", proc->pid, arg_num);
	if (arg_num == -1) {	/* return value */
		trace_user_read(proc, 4 * EAX, &w);
	} else {
		/* get stack pointer */
		trace_user_read(proc, 4 * UESP, &sp);
		trace_mem_read(proc, (void *)sp + 4 * (arg_num + 1), w_bytes, sizeof(long));
	}

	return w;
}
