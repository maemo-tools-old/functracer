#include <errno.h>
#include <error.h> /* TODO use our error functions */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <linux/ptrace.h> /* needs to come after sys/ptrace.h */

#include "breakpoint.h"
#include "callback.h"
#include "debug.h"
#include "process.h"
#include "syscall.h"
#include "trace.h"
#include "util.h"

struct event {
	struct process *proc;
	enum {
		EV_BREAKPOINT,		/* breakpoint generated by functracker */
		EV_NOCHILD,		/* no more children */
		EV_PROC_EXIT,		/* process exited */
		EV_PROC_EXIT_SIGNAL,	/* process killed by a signal */
		EV_PROC_NEW,		/* new process created or attached */
		EV_SIGNAL,		/* process received a signal */
		EV_SYSCALL,		/* syscall entry */
		EV_SYSRET,		/* syscall return */
		EV_PTRACE,		/* ptrace() event */
		EV_UNKNOWN,		/* unknown event */
	} type;
	union {
		int retval;	/* EV_PROC_EXIT */
		int signo;	/* EV_SIGNAL, EV_PROC_EXIT_SIGNAL */
		int sysno;	/* EV_SYSCALL, EV_SYSRET */
		void *addr;	/* EV_BREAKPOINT */
		pid_t pid;	/* EV_PROC_NEW */
	} data;
};

#if 0 /* TODO: fork() tracing not implemented yet */
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
#endif



static pid_t ft_wait(int *status)
{
	errno = 0;
	pid_t pid = waitpid(-1, status, __WALL);
	if (pid == -1 && errno != ECHILD)
		error_exit("waitpid");
	return pid;
}

static int wait_for_event(struct event *event)
{
	pid_t pid;
	int status;

	pid = ft_wait(&status);
	if (pid == -1) {
		if (errno == ECHILD) {
			event->type = EV_NOCHILD;
			return 0;
		}
		return -1;
	}
	event->proc = process_from_pid(pid);
	if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP
	    && !event->proc) {
		event->type = EV_PROC_NEW;
		event->data.pid = pid;
	} else if (!event->proc) {
		debug(1, "PID %d not stopped, execve() probably failed", pid);
		return -1;
	} else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP
		   && (status >> 16)) {
		event->type = EV_PTRACE;
	} else if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80)) {
		switch (get_syscall_nr(event->proc, &event->data.sysno)) {
		case 1:
			event->type = EV_SYSCALL;
			break;
		case 2:
			event->type = EV_SYSRET;
			break;
		default:
			return -1;
		}
	} else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
		int ret;

		event->type = EV_BREAKPOINT;
		if ((ret = bkpt_get_address(event->proc,
		    &event->data.addr)) < 0)
			return ret;
	} else if (WIFEXITED(status)) {
		event->type = EV_PROC_EXIT;
		event->data.retval = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		event->type = EV_PROC_EXIT_SIGNAL;
		event->data.signo = WTERMSIG(status);
	} else if (WIFSTOPPED(status)) {
		event->type = EV_SIGNAL;
		event->data.signo = WSTOPSIG(status);
	} else
		event->type = EV_UNKNOWN;

	return 0;
}

static void continue_process(struct process *proc)
{
	int op = bkpt_pending(proc) ? PTRACE_SINGLESTEP : PTRACE_SYSCALL;

	errno = 0;
	if (ptrace(op, proc->pid, 0, 0) == -1 && errno) {
		error_exit("ptrace: %s", bkpt_pending(proc) ? "PTRACE_SINGLESTEP" : "PTRACE_SYSCALL");
	}
}

static void continue_after_signal(pid_t pid, int signo)
{
	if (ptrace(PTRACE_SYSCALL, pid, 0, signo) == -1 && errno) {
		error_exit("ptrace: PTRACE_SYSCALL");
	}
}

static void trace_set_options(pid_t pid)
{
	long options =
	    PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
	    PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC;

	errno = 0;
	if (ptrace(PTRACE_SETOPTIONS, pid, 0, options) == -1
	    && ptrace(PTRACE_OLDSETOPTIONS, pid, 0, options) == -1
	    && errno) {
		error_exit("ptrace: PTRACE_SETOPTIONS");
	}
}

static int dispatch_event(struct event *event)
{
	struct callback *cb = cb_get();

	switch (event->type) {
	case EV_NOCHILD:
		debug(2, "no more children");
		break;
	case EV_PROC_NEW:
		event->proc = add_process(event->data.pid);
		if (cb && cb->process.create)
			cb->process.create(event->proc);
		bkpt_init(event->proc);
		trace_set_options(event->proc->pid);
		continue_process(event->proc);
		break;
	case EV_PROC_EXIT:
		if (cb && cb->process.exit)
			cb->process.exit(event->proc, event->data.retval);
		remove_process(event->proc);
		break;
	case EV_PROC_EXIT_SIGNAL:
		if (cb && cb->process.kill)
			cb->process.kill(event->proc, event->data.signo);
		remove_process(event->proc);
		break;
	case EV_SYSCALL:
		if (cb && cb->syscall.enter)
			cb->syscall.enter(event->proc, event->data.sysno);
		continue_process(event->proc);
		break;
	case EV_SYSRET:
		if (cb && cb->syscall.exit)
			cb->syscall.exit(event->proc, event->data.sysno);
		continue_process(event->proc);
		break;
	case EV_PTRACE:
		debug(2, "ptrace() event (pid=%d)", event->proc->pid);
		continue_process(event->proc);
		break;
	case EV_BREAKPOINT:
		bkpt_handle(event->proc, event->data.addr);
		continue_process(event->proc);
		break;
	case EV_SIGNAL:
		if (cb && cb->process.signal)
			cb->process.signal(event->proc, event->data.signo);
		if (event->data.signo == SIGUSR1 || event->data.signo == SIGUSR2)
			continue_process(event->proc);
		else
			continue_after_signal(event->proc->pid, event->data.signo);
		break;
	default:
		msg_err("an unknown event was returned");
		return -1;
	}

	return 0;
}

static void trace_me(void)
{
	errno = 0;
	if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1 && errno) {
		error_exit("ptrace: PTRACE_TRACEME");
	}
}

int trace_main_loop(void)
{
	struct event event;

	while (wait_for_event(&event) == 0 && dispatch_event(&event) == 0) {
		if (event.type == EV_NOCHILD)
			return 0;
	}

	return -1;
}

int trace_execute(char *filename, char *argv[])
{
	pid_t pid;

	pid = fork();
	if (pid == -1) {
		error_file(filename, "could not execute program");
		return -1;
	} else if (pid == 0) {	/* child */
		trace_me();
		execvp(filename, argv);
		error_file(filename, "could not execute program");
		return -1;
	}

	return 0;
}

int trace_attach(pid_t pid)
{
	debug(1, "stub");

	return 0;
}
