#include <sys/wait.h>
#include <errno.h>

#include "breakpoint.h"
#include "debug.h"
#include "event.h"
#include "process.h"
#include "ptrace.h"
#include "report.h"
#include "syscall.h"
#include "elf.h"

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
		if ((ret = get_breakpoint_address(event->proc,
		    &event->data.addr)) < 0)
			return ret;
		event->data.addr -= DECR_PC_AFTER_BREAK;
	} else if (WIFEXITED(status)) {
#if 0 /* sanity check */
		if (!(kill(event->proc->pid, 0) < 0 && errno == ESRCH))
			debug(1, "dangling process %d", pid);
#endif
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

static int handle_event(struct event *event)
{
	switch (event->type) {
	case EV_NOCHILD:
		debug(1, "no more children");
		break;
	case EV_PROC_NEW:
		debug(1, "new process (PID %d)", event->data.pid);
		event->proc = add_process(event->data.pid);
		event->proc->symbols = read_elf(event->proc);
		register_alloc_breakpoints(event->proc);
		ll_init();
		trace_set_options(event->proc);
		continue_process(event->proc);
		break;
	case EV_PROC_EXIT:
		debug(1, "process exited (PID %d, status %d)",
		    event->proc->pid, event->data.retval);
		ll_trace_signal(-1);
		remove_process(event->proc);
		break;
	case EV_PROC_EXIT_SIGNAL:
		debug(1, "process killed by signal (PID %d, signo %d)",
		    event->proc->pid, event->data.signo);
		remove_process(event->proc);
		break;
	case EV_SYSCALL:
		debug(1, "syscall entry (PID %d, sysno %d)", event->proc->pid,
		    event->data.sysno);
		continue_process(event->proc);
		break;
	case EV_SYSRET:
		debug(1, "syscall exit (PID %d, sysno %d)", event->proc->pid,
		    event->data.sysno);
		continue_process(event->proc);
		break;
	case EV_PTRACE:
		debug(1, "ptrace() event (PID %d)", event->proc->pid);
		continue_process(event->proc);
		break;
	case EV_BREAKPOINT:
		debug(1, "breakpoint (PID %d, addr %p)", event->proc->pid,
		    event->data.addr);
		process_breakpoint(event->proc, event->data.addr);
		continue_process(event->proc);
		break;
	case EV_SIGNAL:
		debug(1, "process received signal (PID %d, signo %d)",
		    event->proc->pid, event->data.signo);
		continue_process(event->proc);
		break;
	default:
		error_msg("an unknown event was returned");
		return -1;
	}

	return 0;
}

int event_loop(void)
{
	struct event event;

	while (wait_for_event(&event) == 0 && handle_event(&event) == 0) {
		if (event.type == EV_NOCHILD)
			return 0;
	}

	return -1;
}
