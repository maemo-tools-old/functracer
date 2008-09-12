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

#define NDEBUG
#include <assert.h>
#include <libiberty.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "debug.h"
#include "function.h"
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
		assert(proc->rp_data == NULL);
		if (rp_init(proc) < 0)
			proc->trace_control = 0;
	}
}

static void process_exit(struct process *proc, int exit_code)
{
	debug(3, "process exited (pid=%d, exit_code=%d)", proc->pid, exit_code);

	if (trace_enabled(proc)) {
		assert(proc->rp_data != NULL);
		if (proc->parent == NULL)
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

	if (signo == SIGUSR1) {
		if (trace_enabled(proc)) {
			assert(proc->rp_data != NULL);
			if (proc->parent == NULL)
				rp_finish(proc->rp_data);
			proc->trace_control = 0;
		} else {
			assert(proc->rp_data == NULL);
			proc->trace_control = rp_init(proc) < 0 ? 0 : 1;
		}
	}
}

static void process_interrupt(struct process *proc)
{
	debug(3, "process interrupted (pid=%d)", proc->pid);
	if (trace_enabled(proc)) {
		assert(proc->rp_data != NULL);
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
	struct rp_data *rd;
	if (proc->parent != NULL)
		rd = proc->parent->rp_data;
	else
		rd = proc->rp_data;

	/* Avoid reporting internal/recursive calls */ 
	if (proc->callstack == NULL || proc->callstack->next != NULL)
		return;

	if (rd != NULL && trace_enabled(proc)) {
		addr_t retval = fn_return_value(proc);
		size_t arg0 = fn_argument(proc, 0);

		assert(proc->rp_data != NULL);
		if (strcmp(name, "__libc_malloc") == 0) {
			rp_malloc(rd, retval, arg0);
		} else if (strcmp(name, "__libc_calloc") == 0) {
			size_t arg1 = fn_argument(proc, 1);
			rp_calloc(rd, retval, arg0, arg1);
		} else if (strcmp(name, "__libc_realloc") == 0) {
			size_t arg1 = fn_argument(proc, 1);
			rp_realloc(rd, arg0, retval, arg1);
		} else if (strcmp(name, "__libc_free") == 0 ) {
			/* Suppress "free(NULL)" calls from trace output. 
			 * They are a no-op according to ISO 
			 */
			if (arg0)
				rp_free(rd, arg0);
		} else if (strcmp(name, "pthread_mutex_lock") == 0 ||
			   strcmp(name, "pthread_mutex_unlock") == 0) {
			debug(3, "pthread function = %s\n", name);
		} else {
			msg_warn("unexpected function exit: name=\"%s\" arg0=%d retval=%#x\n", name,
				arg0, retval);
		}

	}
}

static int library_match(const char *libname, const char *symname)
{
	debug(3, "library symbol match test (libname=\"%s\", symname=\"%s\")",
	      libname, symname);

	return (strcmp(symname, "__libc_calloc") == 0
		|| strcmp(symname, "__libc_malloc") == 0
		|| strcmp(symname, "__libc_free") == 0
		|| strcmp(symname, "__libc_realloc") == 0
		|| strcmp(symname, "pthread_mutex_lock") == 0
		|| strcmp(symname, "pthread_mutex_unlock") == 0);
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
}

struct callback *cb_get(void)
{
	return current_cb;
}
