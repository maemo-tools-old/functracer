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

#ifndef TT_SYSCALL_H
#define TT_SYSCALL_H

#include "process.h"

struct syscall_data {
	/* syscall + breakpoint instructions */
	const unsigned char insns[8];
	/* registers were arguments (up to six) will be stored */
	const unsigned int regs[6];
	/* register where instruction pointer (program counter) is stored */
	const unsigned int ip_reg;
	/* register where system call number is stored */
	const unsigned int sysnum_reg;
	/* register where return value is stored */
	const unsigned int retval_reg;
};

extern struct syscall_data *get_syscall_data(struct process *proc);
extern int get_syscall_nr(struct process *proc, int *nr);
extern long get_syscall_arg(struct process *proc, int arg_num);

#endif /* TT_SYSCALL_H */
