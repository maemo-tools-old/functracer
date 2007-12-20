#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "ptrace.h"
#include "debug.h"

void continue_process(struct process *proc)
{
	if (ptrace(PTRACE_SYSCALL, proc->pid, 0, 0) < 0) {
		perror("ptrace: PTRACE_SYSCALL");
		exit(EXIT_FAILURE);
	}
}

void trace_me(void)
{
	if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
		perror("ptrace: PTRACE_TRACEME");
		exit(EXIT_FAILURE);
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
        if (waitpid (pid, NULL, 0) != pid) {
                perror ("trace_pid: waitpid");
                exit (1);
        }

        return 0;
}
#endif

void trace_set_options(struct process *proc)
{	
	long options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC;
	if (ptrace(PTRACE_SETOPTIONS, proc->pid, 0, options) < 0 &&
	    ptrace(PTRACE_OLDSETOPTIONS, proc->pid, 0, options) < 0) {
		perror("PTRACE_SETOPTIONS");
		exit(EXIT_FAILURE);
	}
}

#define LIST_SIZE(x)	(sizeof(x) / sizeof(x[0]))

int fork_event(struct process *proc, int event, pid_t *new_pid)
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
			return ptrace(PTRACE_GETEVENTMSG, proc->pid, 0, new_pid);
		}
	}

	*new_pid = 0;
	return 0;
}
