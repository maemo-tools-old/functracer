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

#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "debug.h"
#include "syscall.h"
#include "target_mem.h"

#define off_r0 0
#define off_r7 28
#define off_ip 48
#define off_pc 60

struct syscall_data syscall_data = {
	/* ef 00 00 00 - swi 0x00000000; e7 f0 01 f0 - bkpt. instruction */
	.insns = { 0x00, 0x00, 0x00, 0xef, 0xf0, 0x01, 0xf0, 0xe7 },
	.regs = { 0, 1, 2, 3, 4, 5 }, /* r0, r1, r2, r3, r4, r5 */
	.ip_reg = 15,		/* pc */
	.sysnum_reg = 7,	/* r7 */
	.retval_reg = 0		/* r0 */
};

int get_syscall_nr(struct process *proc, int *nr)
{
	/* get the user's pc (plus 8) */
	int pc = trace_user_readw(proc, off_pc);
	/* fetch the SWI instruction */
	int insn = trace_mem_readw(proc, (addr_t)pc - 4);
	int ip = trace_user_readw(proc, off_ip);

	if (insn == 0xef000000 || insn == 0x0f000000) {
		/* EABI syscall */
		*nr = trace_user_readw(proc, off_r7);
	} else if ((insn & 0xfff00000) == 0xef900000) {
		/* old ABI syscall */
		*nr = insn & 0xfffff;
	} else {
		/* FIXME: handle swi<cond> variations */
		/* one possible reason for getting in here is that we
		 * are coming from a signal handler, so the current
		 * PC does not point to the instruction just after the
		 * "swi" one. */
		msg_err("unexpected instruction 0x%x at 0x%x", insn, pc - 4);
		return -1;
	}
	if ((*nr & 0xf0000) == 0xf0000) {
		/* arch-specific syscall */
		*nr &= ~0xf0000;
	}
	/* ARM syscall convention: on syscall entry, ip is zero;
	 * on syscall exit, ip is non-zero */
	return ip ? 2 : 1;
}

long get_syscall_arg(struct process *proc, int arg_num)
{
	if (arg_num == -1)	/* return value */
		return trace_user_readw(proc, off_r0);

	return trace_user_readw(proc, 4 * arg_num);
}
