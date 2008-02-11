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

#include "breakpoint.h"
#include "debug.h"

#define off_pc 60

addr_t bkpt_get_address(struct process *proc)
{
	return trace_user_readw(proc, off_pc);
}

void set_instruction_pointer(struct process *proc, addr_t addr)
{
	trace_user_writew(proc, off_pc, (long)addr);
}

struct bkpt_insn *breakpoint_instruction(addr_t addr)
{
	/* ARM mode breakpoint */
	static struct bkpt_insn arm_insn = {
		.value = { 0xf0, 0x01, 0xf0, 0xe7 },
		.size = 4,
	};
	/* Thumb mode breakpoint */
	static struct bkpt_insn thumb_insn = {
		.value = { 0x01, 0xde },
		.size = 2,
	};

	return (addr & 1) ? &thumb_insn : &arm_insn;
}

addr_t fixup_address(addr_t addr)
{
	debug(3, "addr=0x%x, thumb=%d", addr, addr & 1);
        /* fixup address for thumb instruction */
	return (addr & 1) ? (addr & (addr_t)~1) : addr;
}
