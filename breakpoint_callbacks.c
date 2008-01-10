#include <string.h>

#include "breakpoint.h"
#include "debug.h"
#include "function.h"
#include "process.h"
#include "report.h"

static void function_enter(struct process *proc, const char *name)
{
	debug(3, "function entry (pid=%d, name=%s)", proc->pid, name);
}

static void function_exit(struct process *proc, const char *name)
{
	void *retval = (void *)fn_return_value(proc);
	long arg0 = fn_argument(proc, 0);

	debug(3, "function return (pid=%d, name=%s)", proc->pid, name);
	rp_new_alloc(proc->rp_data, retval, arg0);
}

static int function_match(struct process *proc, const char *name)
{
	debug(3, "function match test (pid=%d, name=%s)", proc->pid, name);

	return (strcmp(name, "malloc") == 0);
}

void bcb_init(void)
{
	struct breakpoint_cb bcb = {
		.function = {
			.enter	= function_enter,
			.exit	= function_exit,
			.match	= function_match,
		},
	};
	bkpt_register_callbacks(&bcb);
}
