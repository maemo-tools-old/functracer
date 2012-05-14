/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <dirent.h>
#include <errno.h>
#include <linux/ptrace.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>

#include "breakpoint.h"
#include "callback.h"
#include "debug.h"
#include "function.h"
#include "process.h"
#include "syscall.h"
#include "trace.h"
#include "util.h"
#include "options.h"

struct event {
	struct process *proc;
	enum {
		EV_BREAKPOINT,	/* breakpoint generated by functracer */
		EV_SINGLESTEP,	/* process is singlestepping */
		EV_NOCHILD,	/* no more children */
		EV_POST_EXIT,	/* process has exited */
		EV_PRE_EXIT,	/* process is exiting and process data is still
				 * available */
		EV_EXIT_SIGNAL,	/* process killed by a signal */
		EV_EXEC,	/* process called exec*() */
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
		pid_t pid;	/* EV_NEW_PROC, EV_FORK, EV_EXEC */
	} data;
};

pid_t ft_waitpid(pid_t pid, int *status, int options)
{
	pid_t pid2;

	errno = 0;
	do {
		pid2 = waitpid(pid, status, options);
	} while (pid2 == -1 && errno == EINTR);

	if (pid2 == -1 && errno != ECHILD)
		error_exit("waitpid");

	return pid2;
}

static int wait_for_event(struct event *event)
{
	pid_t pid;
	int status;

	pid = ft_waitpid(-1, &status, __WALL);
	if (pid == -1) {
		if (errno == ECHILD) {
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
		case PTRACE_EVENT_EXEC:
			event->type = EV_EXEC;
			event->data.pid = pid;
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
		if (event->proc->singlestep)
			event->type = EV_SINGLESTEP;
		else
			event->type = EV_BREAKPOINT;
		event->data.addr = bkpt_get_address(event->proc);
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

	if (proc->singlestep)
		xptrace(FT_PTRACE_SINGLESTEP, proc->pid, NULL, NULL);
	else
		xptrace(PTRACE_CONT, proc->pid, NULL, NULL);
}

static void continue_after_signal(struct process *proc, int signo)
{
	xptrace(PTRACE_CONT, proc->pid, NULL, (void *)signo);
}

static void trace_set_options(pid_t pid)
{
	long options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK
	    | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC
	    | PTRACE_O_TRACEEXIT;

	debug(3, "pid=%d", pid);
	xptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)options);
}

static void trace_detach(pid_t pid)
{
	xptrace(PTRACE_DETACH, pid, NULL, NULL);
}

static int new_process(struct process *parent_proc, pid_t child_pid)
{
	if (arguments.stopping) {
		trace_detach(child_pid);
		return 0;
	}
	struct process *child_proc;
	struct callback *cb = cb_get();

	child_proc = add_process(child_pid);
	if (cb && cb->process.create)
		cb->process.create(child_proc);
	/* At the time handle_child_process() is called, the breakpoints are
	 * enabled on the child process even if it does not share the address
	 * space with the parent (i.e. it is not a thread). This happens
	 * because at the time the children was forked/cloned, the breakpoints
	 * were enabled and were copied to the new process' address space.
	 * Therefore, we disable these breakpoints for the new process. */
	if (parent_proc && (child_proc->parent == NULL)) {
		child_proc->shared = parent_proc->shared;
		/* FIXME: should we call fn_callstack_restore() as done in
		 * handle_interrupt() ? */
		disable_all_breakpoints(child_proc);
		child_proc->shared = NULL;
		if (cb && cb->process.fork) {
			/* Callback should be called only when child is not a
			 * thread. */
			cb->process.fork(parent_proc, child_pid);
		}
	} 
	bkpt_init(child_proc);
	trace_set_options(child_pid);
	continue_process(child_proc);
	return 0;
}

static int handle_child_process(struct process *parent_proc, pid_t child_pid)
{
	int status, pid;

	if (process_from_pid(child_pid)) {
	        return 0;
	}
	pid = ft_waitpid(child_pid, &status, __WALL);
	if (pid == -1) {
		msg_err("waitpid: error waiting for new child");
		return -1;
	}
	if (pid != child_pid) {
		msg_err("waitpid: unexpected PID %d returned", pid);
		return -1;
	}
	if (!(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP)) {
		msg_err("waitpid %d: unexpected status 0x%x returned", child_pid, status);
		return -1;
	}
	return new_process(parent_proc, child_pid);
}

static int new_thread(pid_t tid, pid_t tgid)
{
	struct process *main_thread = process_from_pid(tgid);

	/* Make sure that main thread is already tracing */
	if (!main_thread) {
		/*
		 * If no, starting trace main thread first.
		 * It should only happens during attaching to threaded application
		 */
		pid_t pid;
		int status;

		/* Getting rid from fork ptrace event of the main thread */
		pid = ft_waitpid(tgid, &status, __WALL);

		/* Check if we got correct event */

		if (pid == -1) {
			msg_err("waitpid: error waiting for main thread");
			return -1;
		}
		if (pid != tgid) {
			msg_err("waitpid: unexpected PID %d returned", pid);
			return -1;
		}
		if (!(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP)) {
			msg_err("waitpid %d: unexpected status 0x%x returned", tgid, status);
			return -1;
		}

		/* Start tracing main thread */
		if (new_process(NULL, tgid))
			return -1;
	}

	return new_process(NULL, tid);
}

static int handle_new_process(pid_t child_pid)
{
	struct process *parent_proc;
	int retval;
	pid_t tgid = get_tgid(child_pid);

	/* If it's not the main thread of the process, we should handle it separately */
	if (child_pid != tgid)
		return new_thread(child_pid, tgid);

	parent_proc = process_from_pid(get_ppid(child_pid));

	/* If the parent process is tracing by functracer, the current process was forked from it */
	if (parent_proc) {
		pid_t pid;
		int status;

		/* Getting rid from fork ptrace event of the parrent */
		pid = ft_waitpid(parent_proc->pid, &status, __WALL);

		/* Check if we got correct event */

		if (pid == -1) {
			msg_err("waitpid: error waiting parent fork event");
			return -1;
		}

		if (pid != parent_proc->pid) {
			msg_err("waitpid: unexpected PID %d returned", pid);
			return -1;
		}

		if (!(WIFSTOPPED(status) && (WSTOPSIG(status) == SIGTRAP)
					&& (status >> 16))) {
			msg_err("waitpid %d: unexpected status 0x%x returned", child_pid, status);
			return -1;
		}

		switch (status >> 16) {
			case PTRACE_EVENT_FORK:
			case PTRACE_EVENT_VFORK:
			case PTRACE_EVENT_CLONE:
				break;
			default:
				msg_err("unexpected ptrace() event %d", status >> 16);
				return -1;
		}
	}

	retval = new_process(parent_proc, child_pid);

	if (parent_proc)
		continue_process(parent_proc);

	return retval;
}



static void trace_finish(struct process *proc)
{
	debug(2, "TRACE FINISH");
	struct callback *cb = cb_get();
	if (cb && cb->process.interrupt)
		cb->process.interrupt(proc);
	if (proc->callstack != NULL) {
		debug(2, "restore callstack");
		fn_callstack_restore(proc, 1);
	}
	disable_all_breakpoints(proc);
	bkpt_finish(proc);
	trace_detach(proc->pid);
}

static int handle_interrupt(struct process *proc, int signo)
{
	if (proc->exiting && signo == SIGSTOP) {
		trace_finish(proc);
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
		handle_new_process(event->data.pid);
		break;
	case EV_EXEC:
		free(event->proc->filename);
		event->proc->filename = name_from_pid(event->proc->pid);
		if (cb && cb->process.exec)
			cb->process.exec(event->proc);
		bkpt_finish(event->proc);
		bkpt_init(event->proc);
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
		event->proc->exiting = 1;
		bkpt_finish(event->proc);
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
	case EV_SINGLESTEP:
		singlestep_handle(event->proc, event->data.addr);
		event->proc->singlestep = 0;
		continue_process(event->proc);
		break;
	case EV_SIGNAL:
		if (event->proc->singlestep)
			singlestep_after_signal(event->proc);
		if (handle_interrupt(event->proc, event->data.signo))
			return 0;
		if (cb && cb->process.signal)
			cb->process.signal(event->proc, event->data.signo);
		if (event->data.signo == SIGUSR1)
			continue_process(event->proc);
		else
			continue_after_signal(event->proc, event->data.signo);
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
		if (setsid() < 0) {
			error_file(filename, "could not create new session");
			return -1;
		}
		xptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execvp(filename, argv);
		error_file(filename, "could not execute program");
		return -1;
	}

	return pid;
}

static void trace_attach(pid_t pid)
{
	xptrace(PTRACE_ATTACH, pid, NULL, NULL);
}

void trace_attach_child(pid_t pid)
{
	char proc_dir[MAXPATHLEN];
	DIR *dir;

	sprintf(proc_dir, "/proc/%d/task", pid);
	dir = opendir(proc_dir);

	if (dir != NULL) {
		 struct dirent *dir_name;
		 int tid;

		 while ((dir_name = readdir(dir)) != NULL) {
		 	if (dir_name->d_fileno == 0 ||
				dir_name->d_name[0] == '.')
				continue;
			tid = atoi(dir_name->d_name);
			if (tid <= 0)
				continue;
			trace_attach(tid);
		}
		closedir(dir);
	}
}
