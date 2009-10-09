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

#ifndef FTK_BREAKPOINT_H
#define FTK_BREAKPOINT_H

#include "arch-defs.h"
#include "process.h"
#include "target_mem.h"

/* maximum breakpoint instruction size for all supported architectures */
#define BREAKPOINT_LENGTH	4

struct bkpt_insn {
	unsigned char value[BREAKPOINT_LENGTH];
	size_t size;
};

struct breakpoint;
struct breakpoint {
	addr_t addr, ssol_addr;
	struct bkpt_insn *insn;
	enum { BKPT_ENTRY, BKPT_RETURN, BKPT_SOLIB, BKPT_SENTINEL } type;
	char *symbol;
	int refcnt;
	int enabled;

	unsigned char orig_insn[MAX_INSN_SIZE];
	void (*ssol_pre_handler)(struct process *proc, struct breakpoint *bkpt);
	void (*ssol_post_handler)(struct process *proc, struct breakpoint *bkpt);
	void *ssol_data;
};

extern void set_instruction_pointer(struct process *proc, addr_t addr);
extern addr_t bkpt_get_address(struct process *proc);
extern struct bkpt_insn *breakpoint_instruction(addr_t addr);
extern addr_t fixup_address(addr_t addr);
extern void bkpt_handle(struct process *proc, addr_t addr);
extern void singlestep_handle(struct process *proc, addr_t addr);
extern void singlestep_after_signal(struct process *proc);
extern void bkpt_init(struct process *proc);
extern void bkpt_finish(struct process *proc);
extern void disable_all_breakpoints(struct process *proc);
extern int ssol_prepare_bkpt(struct breakpoint *bkpt, void *safe_insn);

#endif /* !FTK_BREAKPOINT_H */
