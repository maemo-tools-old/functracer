/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
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

#include <libiberty.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "debug.h"
#include "function.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "target_mem.h"

static struct callback *current_cb = NULL;

static int trace_enabled(struct process *proc)
{
	return (proc->trace_control != 0);
}

static void process_create(struct process *proc)
{
	debug(3, "new process (pid=%d)", proc->pid);

	if (trace_enabled(proc)) {
		if (rp_init(proc) < 0) {
			proc->trace_control = 0;
		} else {
			rp_event(proc, "Process %d was created\n", proc->pid);
		}
	}
}

static void process_exit(struct process *proc, int exit_code)
{
	debug(3, "process exited (pid=%d, exit_code=%d)", proc->pid, exit_code);

	if (trace_enabled(proc)) {
		rp_event(proc, "Process %d has exited with code %d\n",
			 proc->pid, exit_code);
		rp_finish(proc);
	}
}

static void process_kill(struct process *proc, int signo)
{
	debug(3, "process killed by signal (pid=%d, signo=%d)", proc->pid, signo);
	if (trace_enabled(proc)) {
		rp_event(proc, "Process %d was killed by signal %d\n",
			 proc->pid, signo);
	}
}

static void toggle_tracing(struct process *proc)
{
	if (trace_enabled(proc)) {
		rp_finish(proc);
		proc->trace_control = 0;
	} else {
		proc->trace_control = rp_init(proc) < 0 ? 0 : 1;
	}
}

static void process_signal(struct process *proc, int signo)
{
	debug(3, "process received signal (pid=%d, signo=%d)", proc->pid, signo);

	if (signo == SIGUSR1) {
		for_each_process(toggle_tracing);
	} else if (trace_enabled(proc)) {
		rp_event(proc, "Process %d received signal %d\n",
			 proc->pid, signo);
	}
}

static void process_interrupt(struct process *proc)
{
	debug(3, "process interrupted (pid=%d)", proc->pid);
	if (trace_enabled(proc)) {
		rp_event(proc, "Process %d was detached\n", proc->pid);
		rp_finish(proc);
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

	/* Avoid reporting internal/recursive calls */ 
	if (proc->callstack == NULL || proc->callstack->next != NULL)
		return;

	if (trace_enabled(proc))
		plg_function_exit(proc, name);
}

static void library_load(struct process *proc, addr_t start_addr,
			 addr_t end_addr, char *path)
{
	debug(3, "library load (pid=%d, start=0x%08x, end=0x%08x, path=%s)",
	      proc->pid, start_addr, end_addr, path);

	if (trace_enabled(proc))
		rp_event(proc, "%s => 0x%08x-0x%08x\n", path, start_addr,
			 end_addr);
}

static void cb_register(struct callback *cb)
{
	if (current_cb == NULL)
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
			.load	   = library_load,
		},
	};
	cb_register(&cb);
	plg_init();
}

void cb_finish(void)
{
	if (current_cb == NULL)
		return;
	free(current_cb);
	current_cb = NULL;
	plg_finish();
}

struct callback *cb_get(void)
{
	return current_cb;
}
