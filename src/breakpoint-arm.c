/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2006, 2007 Motorola Inc.
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
 * Branch displacement calculation based on code from the Linux kernel
 * (arch/arm/kernel/kprobes-decode.c)
 */

#include <linux/ptrace.h>

#include "breakpoint.h"
#include "config.h"
#include "debug.h"

#define off_r0 0
#define off_pc 60
#define off_cpsr 64

#define ARM_Rn(insn)		(insn >> 16 & 0xf)
#define ARM_Rd(insn)		(insn >> 12 & 0xf)
#define ARM_cc(insn)		(insn >> 28 & 0xf)
#define ARM_reglist(insn, Reg)	(insn & (1 << Reg))
#define ARM_PC			15
#define STM(insn)		((insn & 0x0e500000) == 0x08000000 && ARM_cc(insn) != 0xf)
#define LDR_imm(insn)		((insn & 0x0f300000) == 0x05100000)
#define MOV_imm(insn)		((insn & 0x0fe00000) == 0x03a00000)
#define ARM_NOP			0xe1a00000	/* nop (mov r0,r0) */

#define BRANCH(insn)		((insn & 0xff000000) == 0xea000000)
#define sign_extend(x, signbit) ((x) | (0 - ((x) & (1 << (signbit)))))
#define branch_displacement(insn) sign_extend(((insn) & 0xffffff) << 2, 25)

addr_t bkpt_get_address(struct process *proc)
{
	return trace_user_readw(proc, off_pc) - DECR_PC_AFTER_BREAK;
}

void set_instruction_pointer(struct process *proc, addr_t addr)
{
	if (addr & 1) {
		/* Set Thumb mode if ADDR is a Thumb address */
		long cpsr;
		addr = fixup_address(addr);
		cpsr = trace_user_readw(proc, off_cpsr);
		cpsr |= (1 << 5);
		trace_user_writew(proc, off_cpsr, cpsr);
	}
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

static void pre_rn_pc(struct process *proc, struct breakpoint *bkpt)
{
	long pc = bkpt->addr;
	long r0 = trace_user_readw(proc, off_r0);
	/* If Rn is PC, the value used is PC + 8 */
	trace_user_writew(proc, off_r0, pc + 8);
	bkpt->ssol_data = (void *)r0;
}

static void post_rn_pc(struct process *proc, struct breakpoint *bkpt)
{
	/* Restore original R0 value */
	long r0 = (long)bkpt->ssol_data;
	trace_user_writew(proc, off_r0, r0);
}

static void post_branch(struct process *proc, struct breakpoint *bkpt)
{
	addr_t pc = bkpt->addr;
	long insn = bkpt->orig_insn;
	int disp = branch_displacement(insn);
	set_instruction_pointer(proc, pc + 8 + disp);
}

int ssol_prepare_bkpt(struct breakpoint *bkpt, long *safe_insn)
{
	/* TODO: add support for more instructions */
	long insn = bkpt->orig_insn;

	/* by default, the instruction is unmodified */
	*safe_insn = insn;

	/* SOLIB breakpoints are handled specially, and their original
	 * instruction is not even executed. */
	if (bkpt->type == BKPT_SOLIB)
		return 0;

	/* Store multiple (Rn != PC and PC not in register list) */
	if (STM(insn) && ARM_Rn(insn) != ARM_PC && !ARM_reglist(insn, ARM_PC)) {
		return 0;
	}
	/* Load immediate offset (Rd != PC and Rn == PC) */
	if (LDR_imm(insn) && ARM_Rd(insn) != ARM_PC && ARM_Rn(insn) == ARM_PC) {
		*safe_insn &= ~(0xf << 16); /* Set Rn to R0 */
		bkpt->ssol_pre_handler = pre_rn_pc;
		bkpt->ssol_post_handler = post_rn_pc;
		return 0;
	}
	/* Move immediate to register (Rd != PC and Rn != PC) */
	if (MOV_imm(insn) && ARM_Rd(insn) != ARM_PC && ARM_Rn(insn) != ARM_PC) {
		return 0;
	}
	/* Branch (always) */
	if (BRANCH(insn)) {
		/* replace instruction with a no-op. It will be simulated on
		 * the pre handler */
		*safe_insn = ARM_NOP;
		bkpt->ssol_post_handler = post_branch;
		return 0;
	}
	return -1;
}
