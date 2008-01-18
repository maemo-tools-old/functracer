#include <errno.h>
#include <linux/ptrace.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "breakpoint.h"
#include "callback.h"
#include "debug.h"
#include "process.h"
#include "syscall.h"
#include "trace.h"
#include "util.h"

int exiting = 0;

struct event {
	struct process *proc;
	enum {
		EV_BREAKPOINT,	/* breakpoint generated by functracker */
		EV_NOCHILD,	/* no more children */
		EV_POST_EXIT,	/* process has exited */
		EV_PRE_EXIT,	/* process is exiting and process data is still
				 * available */
		EV_EXIT_SIGNAL,	/* process killed by a signal */
		EV_FORK,	/* process forked a child */
		EV_NEW_PROC,	/* new process created or attached */
		EV_SIGNAL,	/* process received a signal */
		EV_SYSCALL,	/* syscall entry */
		EV_SYSRET,	/* syscall return */
	} type;
	union {
		int retval;	/* EV_PRE_EXIT, EV_POST_EXIT */
		int signo;	/* EV_SIGNAL, EV_EXIT_SIGNAL */
		int sysno;	/* EV_SYSCALL, EV_SYSRET */
		addr_t addr;	/* EV_BREAKPOINT */
		pid_t pid;	/* EV_NEW_PROC, EV_FORK */
	} data;
};

static pid_t ft_wait(int *status)
{
	pid_t pid;

	errno = 0;
	pid = waitpid(-1, status, __WALL);
	debug(5, "waitpid: pid=%d, status=0x%x", pid, *status);
	if (pid == -1 && errno != ECHILD && errno != EINTR)
		error_exit("waitpid");
	return pid;
}

static int wait_for_event(struct event *event)
{
	pid_t pid;
	int status;

	pid = ft_wait(&status);
	if (pid == -1) {
		if (errno == ECHILD || errno == EINTR) {
			event->type = EV_NOCHILD;
			return 0;
		}
		return -1;
	}
	event->proc = process_from_pid(pid);
	if (WIFSTOPPED(status) && (WSTOPSIG(status) == SIGTRAP
	    || WSTOPSIG(status) == SIGSTOP) && !event->proc) {
		event->type = EV_NEW_PROC;
		event->data.pid = pid;
	} else if (!event->proc) {
		debug(1, "PID %d not stopped, execve() probably failed", pid);
		return -1;
	} else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP
		   && (status >> 16)) {
		switch (status >> 16) {
		case PTRACE_EVENT_EXIT:
			event->type = EV_PRE_EXIT;
			xptrace(PTRACE_GETEVENTMSG, pid, NULL, &event->data.retval);
			break;
		case PTRACE_EVENT_FORK:
		case PTRACE_EVENT_VFORK:
		case PTRACE_EVENT_CLONE:
			event->type = EV_FORK;
			xptrace(PTRACE_GETEVENTMSG, pid, NULL, &event->data.pid);
			break;
		default:
			msg_err("unexpected ptrace() event %d", status >> 16);
			return -1;
		}
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
		event->type = EV_POST_EXIT;
		event->data.retval = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		event->type = EV_EXIT_SIGNAL;
		event->data.signo = WTERMSIG(status);
	} else if (WIFSTOPPED(status)) {
		event->type = EV_SIGNAL;
		event->data.signo = WSTOPSIG(status);
	} else {
		return -1;
	}

	return 0;
}

static void continue_process(struct process *proc)
{
	debug(3, "pid=%d", proc->pid);

	if (bkpt_pending(proc))
		xptrace(PTRACE_SINGLESTEP, proc->pid, NULL, NULL);
	else
		xptrace(PTRACE_SYSCALL, proc->pid, NULL, NULL);
}

static void continue_after_signal(pid_t pid, int signo)
{
	xptrace(PTRACE_SYSCALL, pid, NULL, (void *)signo);
}

static void trace_set_options(pid_t pid)
{
	long options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK
	    | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC
	    | PTRACE_O_TRACEEXIT;

	debug(3, "pid=%d", pid);
	xptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)options);
}

static int handle_child_process(struct process *parent_proc, pid_t child_pid)
{
	struct callback *cb = cb_get();
	struct process *child_proc = add_process(child_pid);
	int status, ret, pid;

	if (cb && cb->process.create)
		cb->process.create(child_proc);
	child_proc->breakpoints = parent_proc->breakpoints;
	pid = waitpid(child_proc->pid, &status, __WALL);
	if (ret == -1) {
		msg_err("waitpid: error waiting for new child");
		return -1;
	}
	if (pid != child_pid) {
		msg_err("waitpid: unexpected PID %d returned", pid);
		return -1;
	}
	if (!(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP)) {
		msg_err("waitpid: unexpected status 0x%x returned", status);
		return -1;
	}
	trace_set_options(child_proc->pid);
	continue_process(child_proc);

	return 0;
}

static void trace_detach(pid_t pid)
{
	xptrace(PTRACE_DETACH, pid, NULL, NULL);
}

static int handle_interrupt(struct process *proc, int signo)
{
	if (exiting && signo == SIGSTOP) {
		pid_t pid = proc->pid;
		disable_all_breakpoints(proc);
		trace_detach(pid);
		remove_process(proc);
		return 1;
	}
	return 0;
}

static int dispatch_event(struct event *event)
{
	struct callback *cb = cb_get();

	switch (event->type) {
	case EV_NOCHILD:
		debug(2, "no more children");
		break;
	case EV_NEW_PROC:
		event->proc = add_process(event->data.pid);
		if (cb && cb->process.create)
			cb->process.create(event->proc);
		bkpt_init(event->proc);
		trace_set_options(event->proc->pid);
		continue_process(event->proc);
		break;
	case EV_FORK:
		handle_child_process(event->proc, event->data.pid);
		continue_process(event->proc);
		break;
	case EV_PRE_EXIT:
		/* Run process.exit() callback on PRE_EXIT event, because at
		 * this time /proc/PID/maps is still available for the process.
		 */
		if (cb && cb->process.exit)
			cb->process.exit(event->proc, event->data.retval);
		continue_process(event->proc);
		break;
	case EV_POST_EXIT:
		remove_process(event->proc);
		break;
	case EV_EXIT_SIGNAL:
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
	case EV_BREAKPOINT:
		bkpt_handle(event->proc, event->data.addr);
		continue_process(event->proc);
		break;
	case EV_SIGNAL:
		if (handle_interrupt(event->proc, event->data.signo)) {
			if (cb && cb->process.interrupt)
				cb->process.interrupt(event->proc);
			return 0;
		}
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

int trace_main_loop(void)
{
	struct event event;

	while (wait_for_event(&event) == 0 && dispatch_event(&event) == 0) {
		if (event.type == EV_NOCHILD)
			return 0;
	}
	debug(1, "event processing error");

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
		xptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execvp(filename, argv);
		error_file(filename, "could not execute program");
		return -1;
	}

	return 0;
}

void trace_attach(pid_t pid)
{
	xptrace(PTRACE_ATTACH, pid, NULL, NULL);
}
