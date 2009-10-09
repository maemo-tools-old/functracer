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
#include <asm/ptrace.h>

#include "breakpoint.h"
#include "debug.h"
#include "function.h"
#include "ssol.h"
#include "target_mem.h"

#define off_pc 60
#define off_lr 56
#define off_sp 52
#define off_r0 0

long fn_argument(struct process *proc, int arg_num)
{
	long w;
	addr_t sp;

	debug(3, "fn_argument(pid=%d, arg_num=%d)", proc->pid, arg_num);
	if (proc->callstack == NULL) {
		debug(1, "no function argument data is saved");
		if (arg_num < 4)
			w = trace_user_readw(proc, 4 * arg_num);
		else {
			sp = trace_user_readw(proc, off_sp);
			w = trace_mem_readw(proc, sp + 4 * (arg_num - 4));
		}
	} else {
		struct pt_regs *regs;
		regs = (struct pt_regs *)proc->callstack->data[0];
		if (arg_num < 4)
			w = regs->uregs[arg_num];
		else {
			sp = regs->ARM_sp;
			w = trace_mem_readw(proc, sp + 4 * (arg_num - 4));
		}
	}

	return w;
}

long fn_return_value(struct process *proc)
{
	return trace_user_readw(proc, off_r0);
}

void fn_get_return_address(struct process *proc, addr_t *addr)
{
	assert(proc->callstack != NULL);
	*addr = (addr_t)proc->callstack->data[1];
}

void fn_set_return_address(struct process *proc, addr_t addr)
{
	trace_user_writew(proc, off_lr, addr);
}

void fn_do_return(struct process *proc)
{
	addr_t ret_addr;

	ret_addr = trace_user_readw(proc, off_lr);
	set_instruction_pointer(proc, ret_addr);
}

int fn_callstack_push(struct process *proc, char *fn_name)
{
	struct callstack *cs;
	struct pt_regs *regs;

	regs = xmalloc(sizeof(struct pt_regs));
	trace_getregs(proc, regs);
	if (regs->ARM_lr == proc->ssol->first) {
		debug(2, "direct branch detected, callee=%s, caller=%s", fn_name,
			proc->callstack ? (char *)proc->callstack->data[2] : "<unknown>");
		free(regs);
		return -1;
	}
	debug(2, "pid=%d, depth=%d", proc->pid, ++proc->callstack_depth);
	cs = xmalloc(sizeof(struct callstack));
	cs->data[0] = (void *) regs;
	cs->data[1] = (void *) regs->ARM_lr;
	cs->data[2] = (void *) fn_name;
	cs->next = proc->callstack;
	proc->callstack = cs;

	return 0;
}

void fn_callstack_pop(struct process *proc)
{
	struct callstack *cs;

	assert(proc->callstack != NULL);
	debug(2, "pid=%d, depth=%d", proc->pid, proc->callstack_depth--);
	cs = proc->callstack->next;
	free(proc->callstack->data[0]);
	free(proc->callstack);
	proc->callstack = cs;
}

void fn_callstack_restore(struct process *proc, int original)
{
	struct callstack *cs = proc->callstack;

	assert(cs != NULL);
	fn_set_return_address(proc, original ? (addr_t)cs->data[1] : proc->ssol->first);
	/* FIXME: Should we skip callstack top? In theory it was just popped
	 * before the function return. */
	while (cs) {
		struct pt_regs *regs = (struct pt_regs *)cs->data[0];
		trace_mem_writew(proc, regs->ARM_sp - 4,
				 original ? (addr_t)cs->data[1] : proc->ssol->first);
		cs = cs->next;
	}
}

char *fn_name(struct process *proc)
{
	assert(proc->callstack != NULL);
	return (char *)proc->callstack->data[2];
}
