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

#include <linux/ptrace.h>

#include "breakpoint.h"
#include "config.h"
#include "debug.h"

addr_t bkpt_get_address(struct process *proc)
{
	addr_t addr;

	debug(3, "pid=%d", proc->pid);
	addr = trace_user_readw(proc, 4 * EIP);
	/* EIP is always incremented by 1 after hitting a breakpoint, so
 	 * decrement it to get the actual breakpoint address. */
	if (!proc->singlestep)
		addr -= DECR_PC_AFTER_BREAK;

	return addr;
}

void set_instruction_pointer(struct process *proc, addr_t addr)
{
	debug(3, "pid=%d, addr=0x%x", proc->pid, addr);
	trace_user_writew(proc, 4 * EIP, (long)addr);
}

struct bkpt_insn *breakpoint_instruction(addr_t addr __unused)
{
	static struct bkpt_insn insn = {
		.value = { 0xcc },
		.size = 1,
	};
	return &insn;
}

addr_t fixup_address(addr_t addr)
{
	/* no fixup needed for x86 */
	return addr;
}
