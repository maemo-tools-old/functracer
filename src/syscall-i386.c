/*
 * This file is part of Functracker.
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

#include <linux/ptrace.h>

#include "syscall.h"
#include "target_mem.h"

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
