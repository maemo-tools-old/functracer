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
 * Branch displacement calculation based on code from the Linux kernel
 * (arch/arm/kernel/kprobes-decode.c)
 */

#include <linux/ptrace.h>
#include <string.h>

#include "breakpoint.h"
#include "debug.h"

#define off_r0 0
#define off_pc 60
#define off_cpsr 64

#define ARM_Rm(insn)		(insn & 0xf)
#define ARM_Rn(insn)		(insn >> 16 & 0xf)
#define ARM_Rd(insn)		(insn >> 12 & 0xf)
#define ARM_cc(insn)		(insn >> 28 & 0xf)
#define ARM_reglist(insn, Reg)	(insn & (1 << Reg))
#define ARM_PC			15
#define STM(insn)		((insn & 0x0e500000) == 0x08000000 && ARM_cc(insn) != 0xf)
#define STR_imm(insn)		((insn & 0x0f100000) == 0x05000000)
#define LDR_imm(insn)		((insn & 0x0f300000) == 0x05100000)
#define MOV_imm(insn)		((insn & 0x0fe00000) == 0x03a00000)
#define MOV_reg(insn)		((insn & 0x0fef0ff0) == 0x01a00000)
/* Same as above, but with SBZ (should be zero) bits not masked. */
/*#define MOV_reg(insn)		((insn & 0x0fe00ff0) == 0x01a00000)*/
#define ARM_NOP			0xe1a00000	/* nop (mov r0,r0) */

#define BRANCH(insn)		((insn & 0xff000000) == 0xea000000)
#define sign_extend(x, signbit) ((x) | (0 - ((x) & (1 << (signbit)))))
#define branch_displacement(insn) sign_extend(((insn) & 0xffffff) << 2, 25)
#define PLD_imm(insn)		((insn & 0xff30f000) == 0xf510f000)
#define CMP_imm(insn)		((insn & 0x0ff0f000) == 0x03500000)
#define SUB_reg(insn)		((insn & 0x0fe00010) == 0x00400000)
#define TST_reg(insn)		((insn & 0x0ff0f010) == 0x01100000)
#define TST_imm(insn)		((insn & 0x0ff0f000) == 0x03100000)
#define RSB_reg(insn)		((insn & 0x0fe00010) == 0x00600000)
#define ORR_reg(insn)		((insn & 0x0fe00010) == 0x01800000)

#define SUB_reg_reg_imm(insn)	((insn & 0x0fe00000) == 0x02400000)
#define LSL_reg_reg_imm(insn)	((insn & 0x0fef0070) == 0x01A00000)

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

static void pre_rn_pc_thumb(struct process *proc, struct breakpoint *bkpt)
{
	long pc = bkpt->addr;
	long r0 = trace_user_readw(proc, off_r0);
	/* If Rn is PC, the value used is Align(PC, 4) + 4 */
	trace_user_writew(proc, off_r0, (pc & ~3) + 4);
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
	long insn = bkpt->orig_insn.insn;
	int disp = branch_displacement(insn);
	set_instruction_pointer(proc, pc + 8 + disp);
}

static int ssol_prepare_bkpt_thumb(struct breakpoint *bkpt, void *safe_insn)
{
	int bits_15_11;
	unsigned short *insn_s = (unsigned short *)safe_insn;
	memcpy(insn_s, &bkpt->orig_insn.insn, 4);

	/* If bits [15:11] of the first halfword are 0x1D, 0x1E or 0x1F,
	   then it is a 32-bit thumb instruction */
	bits_15_11 = (insn_s[0] >> 11) & ((1 << 5) - 1);
	if (bits_15_11 >= 0x1D && bits_15_11 <= 0x1F)
	{
		/* 32-bit thumb instruction */

		memcpy(&insn_s[2], bkpt->insn->value, bkpt->insn->size);

		if ((insn_s[0] == 0xE92D) && ((insn_s[1] & 0xA000) == 0)) {
			/* PUSH {register_list} */
			return 0;
		}
		if (((insn_s[0] & 0xFF80) == 0xFB00) &&
		    ((insn_s[0] & 0x000F) != 0x000F) &&
		    ((insn_s[1] & 0x000F) != 0x000F) &&
		    ((insn_s[1] & 0x0F00) != 0x0F00)) {
			/* MUL/MLA variants (regs != PC) */
			return 0;
		}
	}
	else
	{
		/* 16-bit thumb instruction */

		memcpy(insn_s, &bkpt->orig_insn.insn, 2);
		memcpy(&insn_s[1], bkpt->insn->value, bkpt->insn->size);

		if ((insn_s[0] & 0xF800) == 0x4800) {
			/* LDR (literal) 16-bit variant */
			unsigned short orig_insn16 = insn_s[0];
			
			/* This is a bit tricky, because we need to replace
			 * 16-bit instruction with a 32-bit one.
			 * The straightforward solution does not work, because
			 * we need the length of the original instruction to
			 * be correctly calculated in "ssol_insn_size" function.
			 * And the length is obtained by calculating the
			 * difference between the breakpoint address and SSOL
			 * area start. Hence just doing a simple 16-bit -> 32-bit
			 * replacement in the SSOL area would result in
			 * incorrectly assuming that the length is 4 bytes.
			 *
			 * So we still keep the breakpoint at insn_s[1],
			 * but branch around it.
			 */
			insn_s[0] = 0xE000; /* branch forward to insn_s[2] */
			insn_s[4] = 0xE7FB; /* branch back to insn_s[1] */

			/* "LDR Rt, [pc, imm8]]" -> "LDR.W Rt, [r0, imm12]]" */
			insn_s[2] = 0xF8D0;
			insn_s[3] = ((orig_insn16 & 0x0700) << 4) +
					(orig_insn16 & 0xFF) * 4;
			bkpt->ssol_pre_handler = pre_rn_pc_thumb;
			if ((orig_insn16 & 0x0700) != 0) {
				/* Destination is not r0, it has to be resoted */
				bkpt->ssol_post_handler = post_rn_pc;
			}
			return 0;
		}
		if ((insn_s[0] & 0xFE00)== 0xB400) {
			/* PUSH {register_list} */
			return 0;
		}
		if ((insn_s[0] & 0xFC00) == 0x4000) {
			/* Data-processing instructions */
			return 0;
		}
		if ((insn_s[0] & 0xC000) == 0x0000) {
			/* Data-processing instructions (immediate) */
			return 0;
		}
		if (((insn_s[0] & 0xFF00) == 0x4600) &&
		    ((insn_s[0] & 0x87) != 0x87) &&
		    ((insn_s[0] & 0x78) != 0x78)) {
			/* MOV reg1, reg2 (reg1 != PC, reg2 != PC) */
			return 0;
		}
	}
	return -1;
}

/* TODO: add support for more instructions */
int ssol_prepare_bkpt(struct breakpoint *bkpt, void *safe_insn)
{
	long orig_insn = bkpt->orig_insn.insn;
	long *insn = (long *)safe_insn;

	/* SOLIB breakpoints are handled specially, and their original
	 * instruction is not even executed. */
	if (bkpt->type == BKPT_SOLIB)
		return 0;

	if (bkpt->insn->size == 2) /* thumb */
		return ssol_prepare_bkpt_thumb(bkpt, safe_insn);

	/* by default, the instruction is unmodified */
	memcpy(insn, &bkpt->orig_insn.insn, 4);
	/* insert a breakpoint after the instruction */
	memcpy(insn + 1, bkpt->insn->value, bkpt->insn->size);

	/* Store multiple (Rn != PC and PC not in register list) */
	if (STM(orig_insn) && ARM_Rn(orig_insn) != ARM_PC &&
	    !ARM_reglist(orig_insn, ARM_PC)) {
		return 0;
	}
	/* Store immediate offset (Rd != PC and Rn != PC) */
	if (STR_imm(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Load immediate offset (Rd != PC and Rn != PC) */
	if (LDR_imm(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Load immediate offset (Rd != PC and Rn == PC) */
	if (LDR_imm(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
	    ARM_Rn(orig_insn) == ARM_PC) {
		*insn = orig_insn & ~(0xf << 16); /* Set Rn to R0 */
		bkpt->ssol_pre_handler = pre_rn_pc;
		bkpt->ssol_post_handler = post_rn_pc;
		return 0;
	}
	/* Move immediate to register (Rd != PC and Rn != PC) */
	if (MOV_imm(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
	    ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Move register to register (Rd != PC, Rm != PC and Rn != PC) */
	if (MOV_reg(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
	    ARM_Rm(orig_insn) != ARM_PC && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Branch (always) */
	if (BRANCH(orig_insn)) {
		/* replace instruction with a no-op. It will be simulated on
		 * the pre handler */
		*insn = ARM_NOP;
		bkpt->ssol_post_handler = post_branch;
		return 0;
	}
	/* Preload Data immediate offset (Rn != PC) */
	if (PLD_imm(orig_insn) && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Compare immediate offset (Rn != PC) */
	if (CMP_imm(orig_insn) && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Subtract register from register (Rd != PC, Rm != PC and Rn != PC) */
	if (SUB_reg(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		 ARM_Rm(orig_insn) != ARM_PC && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Compare immediate offset (Rn != PC) */
	if (TST_imm(orig_insn) && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* Reverse Subtract register from register (Rd != PC, Rm != PC and Rn != PC) */
	if (RSB_reg(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		 ARM_Rm(orig_insn) != ARM_PC && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	/* ORR register with register (Rd != PC, Rm != PC and Rn != PC) */
	if (ORR_reg(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		 ARM_Rm(orig_insn) != ARM_PC && ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	if (SUB_reg_reg_imm(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		 ARM_Rn(orig_insn) != ARM_PC) {
		return 0;
	}
	if (LSL_reg_reg_imm(orig_insn) && ARM_Rd(orig_insn) != ARM_PC &&
		 ARM_Rm(orig_insn) != ARM_PC) {
		return 0;
	}
	return -1;
}
