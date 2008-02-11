#define NDEBUG
#include <assert.h>
#include <libiberty.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "debug.h"
#include "function.h"
#include "options.h"
#include "process.h"
#include "report.h"
#include "target_mem.h"

static struct callback *current_cb = NULL;
static int trace_control = 0;

static int trace_enabled(void)
{
	return (trace_control != 0);
}

static void process_create(struct process *proc)
{
	debug(3, "new process (pid=%d)", proc->pid);

	if (trace_enabled()) {
		assert(proc->rp_data == NULL);
		rp_init(proc);
	}
}

static void process_exit(struct process *proc, int exit_code)
{
	debug(3, "process exited (pid=%d, exit_code=%d)", proc->pid, exit_code);

	if (trace_enabled()) {
		assert(proc->rp_data != NULL);
		rp_dump(proc->rp_data);
		rp_finish(proc->rp_data);
	}
}

static void process_kill(struct process *proc, int signo)
{
	debug(3, "process killed by signal (pid=%d, signo=%d)", proc->pid, signo);
}

static void process_signal(struct process *proc, int signo)
{
	debug(3, "process received signal (pid=%d, signo=%d)", proc->pid, signo);

	if (signo == SIGUSR1 || signo == SIGUSR2) {
		if (trace_enabled()) {
			assert(proc->rp_data != NULL);
			rp_dump(proc->rp_data);
			if (signo == SIGUSR1) {
				rp_finish(proc->rp_data);
				trace_control = 0;
			}
		} else {
			assert(proc->rp_data == NULL);
			rp_init(proc);
			trace_control = 1;
		}
	}
}

static void process_interrupt(struct process *proc)
{
	debug(3, "process interrupted (pid=%d)", proc->pid);
	if (trace_enabled()) {
		assert(proc->rp_data != NULL);
		rp_dump(proc->rp_data);
		rp_finish(proc->rp_data);
	}
}

static void syscall_enter(struct process *proc, int sysno)
{
	debug(3, "syscall entry (pid=%d, sysno=%d)", proc->pid, sysno);
}

static void syscall_exit(struct process *proc, int sysno)
{
	debug(3, "syscall exit (pid=%d, sysno=%d)", proc->pid, sysno);
}

static void function_enter(struct process *proc, const char *name)
{
	debug(3, "function entry (pid=%d, name=%s)", proc->pid, name);
}

static void function_exit(struct process *proc, const char *name)
{

	debug(3, "function return (pid=%d, name=%s)", proc->pid, name);
	
	if (trace_enabled()) {
		addr_t retval = fn_return_value(proc);
		size_t arg0 = fn_argument(proc, 0);

		assert(proc->rp_data != NULL);
		if (strcmp(name, "malloc") == 0) {
			rp_new_alloc(proc->rp_data, retval, arg0);
		} else if (strcmp(name, "calloc") == 0) {
			size_t arg1 = fn_argument(proc, 1);
			rp_new_alloc(proc->rp_data, retval, arg0 * arg1);
		} else if (strcmp(name, "realloc") == 0) {
			size_t arg1 = fn_argument(proc, 1);
			rp_new_alloc(proc->rp_data, retval, arg1);
		}
	}
}

static int library_match(const char *name)
{
	debug(3, "library symbol match test (name=%s)", name);

	return (strcmp(name, "calloc") == 0
		|| strcmp(name, "malloc") == 0 
		/* FIXME: realloc() tracking does not work on ARM (causes
		 * recursive loop). */
		/*|| strcmp(name, "realloc") == 0*/
	);
}

static void cb_register(struct callback *cb)
{
	if (current_cb)
		free(current_cb);
	current_cb = xmalloc(sizeof(struct callback));
	memcpy(current_cb, cb, sizeof(struct callback));
}

void cb_init(void)
{
	struct callback cb = {
		.process = {
			.create	   = process_create,
			.exit	   = process_exit,
			.kill	   = process_kill,
			.signal	   = process_signal,
			.interrupt = process_interrupt,
		},
		.syscall = {
			.enter	   = syscall_enter,
			.exit	   = syscall_exit,
		},
		.function = {
			.enter	   = function_enter,
			.exit	   = function_exit,
		},
		.library = {
			.match     = library_match,
		},
	};
	cb_register(&cb);
	if (arguments.enabled != 0)
		trace_control = 1;
}

struct callback *cb_get(void)
{
	return current_cb;
}
