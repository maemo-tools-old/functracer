#include "debug.h"
#include "process.h"
#include "report.h"
#include "trace.h"

static void process_create(struct process *proc)
{
	debug(3, "new process (pid=%d)", proc->pid);

	proc->symbols = read_elf(event->proc);
	proc->rp_data = rp_init(proc->pid);
}

static void process_exit(struct process *proc, int exit_code)
{
	debug(3, "process exited (pid=%d, exit_code=%d)", proc->pid, exit_code);

	rp_dump(proc->rp_data);
	rp_finish(proc->rp_data);
}

static void process_kill(struct process *proc, int signo)
{
	debug(3, "process killed by signal (pid=%d, signo=%d)", proc->pid, signo);
}

static void process_signal(struct process *proc, int signo)
{
	debug(3, "process received signal (pid=%d, signo=%d)", proc->pid, signo);
}

static void syscall_enter(struct process *proc, int sysno)
{
	debug(3, "syscall entry (pid=%d, sysno=%d)", proc->pid, sysno);
}

static void syscall_exit(struct process *proc, int sysno)
{
	debug(3, "syscall exit (pid=%d, sysno=%d)", proc->pid, sysno);
}

void trace_callbacks_init(void)
{
	struct trace_cb tcb = {
		.process = {
			.create	= process_create,
			.exit	= process_exit,
			.kill	= process_kill,
			.signal	= process_signal,
		},
		.syscall = {
			.enter	= syscall_enter,
			.exit	= syscall_exit,
		}
	};
	trace_register_callbacks(&tcb);
}
