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

#include <linux/ptrace.h>

#include "syscall.h"
#include "target_mem.h"

struct syscall_data syscall_data_i386 = {
	/* cd 80 - int 0x80; cc - breakpoint instruction */
	.insns = { 0xcd, 0x80, 0xcc },
	.regs = { EBX, ECX, EDX, ESI, EDI, EBP },
	.ip_reg = EIP,
	.sysnum_reg = EAX,
	.retval_reg = EAX
};

struct syscall_data *get_syscall_data(struct process *proc)
{
	return &syscall_data_i386;
}

int get_syscall_nr(struct process *proc, int *nr)
{
	*nr = trace_user_readw(proc, 4 * ORIG_EAX);
	if (proc->in_syscall) {
		proc->in_syscall = 0;
		return 2;
	}
	if (*nr >= 0) {
		proc->in_syscall = 1;
		return 1;
	}
	return 0;
}

long get_syscall_arg(struct process *proc, int arg_num)
{
	if (arg_num == -1)	/* return value */
		return trace_user_readw(proc, 4 * EAX);

	return trace_user_readw(proc, 4 * arg_num);
}
