#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <errno.h>

#include "breakpoint.h"
#include "debug.h"
#include "ptrace.h"

void continue_process(struct process *proc)
{
	errno = 0;
	int op = pending_breakpoint(proc) ? PTRACE_SINGLESTEP : PTRACE_SYSCALL;
	if (ptrace(op, proc->pid, 0, 0) == -1 && errno) {
		error_exit("ptrace: %s", pending_breakpoint(proc) ? "PTRACE_SINGLESTEP" : "PTRACE_SYSCALL");
	}
}

void continue_after_signal(struct process *proc, int signo)
{
	if (ptrace(PTRACE_SYSCALL, proc->pid, 0, signo) == -1 && errno) {
		error_exit("ptrace: PTRACE_SYSCALL");
	}
}

void trace_me(void)
{
	errno = 0;
	if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1 && errno) {
		error_exit("ptrace: PTRACE_TRACEME");
	}
}

void trace_set_options(struct process *proc)
{
	long options =
	    PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
	    PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC;

	errno = 0;
	if (ptrace(PTRACE_SETOPTIONS, proc->pid, 0, options) == -1
	    && ptrace(PTRACE_OLDSETOPTIONS, proc->pid, 0, options) == -1
	    && errno) {
		error_exit("ptrace: PTRACE_SETOPTIONS");
	}
}

#define LIST_SIZE(x)	(sizeof(x) / sizeof(x[0]))

int is_fork(struct process *proc, int event)
{
	int i;
	struct init_fork_events {
		int event;
		const char *name;
	} fork_events[] = {
		{ PTRACE_EVENT_FORK, "PTRACE_EVENT_FORK" },
		{ PTRACE_EVENT_VFORK, "PTRACE_EVENT_VFORK" },
		{ PTRACE_EVENT_CLONE, "PTRACE_EVENT_CLONE" },
	};

	for (i = 0; i < LIST_SIZE(fork_events); i++) {
		struct init_fork_events *ptr = fork_events + i;

		if (event == ptr->event) {
			debug(1, "detected fork (%s)", ptr->name);
			return 1;
		}
	}

	return 0;
}

void get_fork_pid(struct process *proc, pid_t *new_pid)
{
	errno = 0;
	if (ptrace(PTRACE_GETEVENTMSG, proc->pid, 0, new_pid) == -1 && errno) {
		error_exit("ptrace: PTRACE_GETEVENTMSG");
	}
	if (*new_pid <= 0) {
		error_exit("invalid PID %d returned by PTRACE_GETEVENTMSG\n",
		    *new_pid);
	}
}

static void trace_mem_io(struct process *proc, void *addr, void *buf, size_t count, int write)
{
	long w;
	unsigned int i, j;
	unsigned char *buf_bytes = buf;
	unsigned char *w_bytes = (unsigned char *)&w;

	debug(3, "trace_mem_io(pid=%d, addr=%p, buf=%p, count=%d, write=%d", proc->pid, addr, buf, count, write);
	for (i = 0; i < 1 + ((count - 1) / sizeof(long)); i++) {
		errno = 0;
		w = ptrace(PTRACE_PEEKTEXT, proc->pid, addr + i * sizeof(long), 0);
		if (w == -1 && errno) {
			error_exit("trace_mem_io (read): ptrace");
		}
		debug(4, "trace_mem_io (read): w = 0x%08lx", w);
		for (j = 0; j < sizeof(long) && i * sizeof(long) + j < count; j++) {
			if (write)
				w_bytes[j] = buf_bytes[i * sizeof(long) + j];
			else
				buf_bytes[i * sizeof(long) + j] = w_bytes[j];
		}
		if (write) {
			errno = 0;
			if (ptrace(PTRACE_POKETEXT, proc->pid, addr + i * sizeof(long), w) == -1 && errno) {
				error_exit("trace_mem_io (write): ptrace");
			}
			debug(4, "trace_mem_io (write): w = 0x%08lx", w);
		}
	}
}

void trace_mem_read(struct process *proc, void *addr, void *buf, size_t count)
{
	trace_mem_io(proc, addr, buf, count, 0);
}

void trace_mem_write(struct process *proc, void *addr, void *buf, size_t count)
{
	trace_mem_io(proc, addr, buf, count, 1);
}

void trace_user_read(struct process *proc, int off, long *val)
{
	errno = 0;
	*val = ptrace(PTRACE_PEEKUSER, proc->pid, off, 0);
	if (*val == -1 && errno)
		error_exit("trace_user_read: ptrace");
}

void trace_user_write(struct process *proc, int off, long val)
{
	errno = 0;
	if (ptrace(PTRACE_POKEUSER, proc->pid, off, val) == -1 && errno)
		error_exit("trace_user_write: ptrace");
}

void trace_getregs(struct process *proc, void *regs)
{
	errno = 0;
	if (ptrace(PTRACE_GETREGS, proc->pid, 0, regs) == -1 && errno)
		error_exit("trace_getregs: ptrace");
}
