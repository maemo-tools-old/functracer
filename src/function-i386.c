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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <libiberty.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "breakpoint.h"
#include "debug.h"
#include "function.h"
#include "ssol.h"

static addr_t get_stack_pointer(struct process *proc)
{
	return trace_user_readw(proc, 4 * UESP);
}

long fn_argument(struct process *proc, int arg_num)
{
	addr_t sp;

	debug(3, "fn_argument(pid=%d, arg_num=%d)", proc->pid, arg_num);
	if (proc->callstack == NULL) {
		debug(1, "callstack is empty");
		sp = get_stack_pointer(proc);
	} else {
		sp = (addr_t)proc->callstack->data[0];
	}
	return trace_mem_readw(proc, sp + 4 * (arg_num + 1));
}

long fn_return_value(struct process *proc)
{
	return trace_user_readw(proc, 4 * EAX);
}

void fn_get_return_address(struct process *proc, addr_t *addr)
{
	assert(proc->callstack != NULL);
	*addr = (addr_t)proc->callstack->data[1];
}

void fn_set_return_address(struct process *proc, addr_t addr)
{
	addr_t sp;

	sp = get_stack_pointer(proc);
	trace_mem_writew(proc, sp, addr);
}

void fn_do_return(struct process *proc)
{
	addr_t sp, ret_addr;

	sp = get_stack_pointer(proc);
	ret_addr = trace_mem_readw(proc, sp);
	trace_user_writew(proc, 4 * UESP, sp - sizeof(long));
	set_instruction_pointer(proc, ret_addr);
}

int fn_callstack_push(struct process *proc, char *fn_name)
{
	struct callstack *cs;
	addr_t sp, ret_addr;

	sp = get_stack_pointer(proc);
	ret_addr = trace_mem_readw(proc, sp);
	if (ret_addr == proc->shared->ssol->first) {
		debug(2, "direct branch detected, callee=%s, caller=%s", fn_name,
			proc->callstack ? (char *)proc->callstack->data[2] : "<unknown>");
		return -1;
	}
	proc->callstack_depth++;
	debug(1, "pid=%d, depth=%d", proc->pid, proc->callstack_depth);
	cs = xmalloc(sizeof(struct callstack));
	cs->data[0] = (void *) sp;
	cs->data[1] = (void *) ret_addr;
	cs->data[2] = (void *) fn_name;
	cs->next = proc->callstack;
	proc->callstack = cs;

	return 0;
}

void fn_callstack_pop(struct process *proc)
{
	struct callstack *cs;

	assert(proc->callstack != NULL);
	proc->callstack_depth--;
	debug(1, "pid=%d, depth=%d", proc->pid, proc->callstack_depth);
	cs = proc->callstack->next;
	free(proc->callstack);
	proc->callstack = cs;
}

void fn_callstack_restore(struct process *proc, int original)
{
	struct callstack *cs = proc->callstack;

	assert(cs != NULL);
	/* The line below was commented out because it actually overwrites the first argument. */
	//fn_set_return_address(proc, original ? (addr_t)cs->data[1] : proc->shared->ssol->first);
	/* FIXME: Should we skip callstack top? In theory it was just popped
	 * before the function return. */
	while (cs) {
		addr_t sp = (addr_t)cs->data[0];
		trace_mem_writew(proc, sp,
				 original ? (addr_t)cs->data[1] : proc->shared->ssol->first);
		cs = cs->next;
	}
}

char *fn_name(struct process *proc)
{
	assert(proc->callstack != NULL);
	return (char *)proc->callstack->data[2];
}
