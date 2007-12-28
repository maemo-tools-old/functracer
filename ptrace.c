#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <errno.h>

#include "ptrace.h"
#include "debug.h"

void continue_process(struct process *proc)
{
	errno = 0;
	if (ptrace(PTRACE_SYSCALL, proc->pid, 0, 0) == -1 && errno) {
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

#if 0
static int trace_pid(pid_t pid)
{
	if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
		return -1;
	}

	/* man ptrace: PTRACE_ATTACH attaches to the process specified
	   in pid.  The child is sent a SIGSTOP, but will not
	   necessarily have stopped by the completion of this call;
	   use wait() to wait for the child to stop. */
	if (waitpid(pid, NULL, 0) != pid) {
		perror("trace_pid: waitpid");
		exit(1);
	}

	return 0;
}
#endif

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
