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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "breakpoint.h"
#include "callback.h"
#include "debug.h"
#include "dict.h"
#include "function.h"
#include "process.h"
#include "solib.h"
#include "target_mem.h"

static void enable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	debug(1, "pid=%d, addr=0x%x", proc->pid, bkpt->addr);

	trace_mem_read(proc, bkpt->addr, bkpt->orig_insn, bkpt->insn->size);
	trace_mem_write(proc, bkpt->addr, bkpt->insn->value, bkpt->insn->size);
}

static void disable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	debug(1, "pid=%d, addr=0x%x", proc->pid, bkpt->addr);

	trace_mem_write(proc, bkpt->addr, bkpt->orig_insn, bkpt->insn->size);
}

static struct breakpoint *breakpoint_from_address(struct process *proc, addr_t addr)
{
	return dict_find_entry(proc->breakpoints, (void *)addr);
}

static void breakpoint_get(struct process *proc, struct breakpoint *bkpt)
{
	bkpt->enabled++;
	if (bkpt->enabled == 1)
		enable_breakpoint(proc, bkpt);
}

static void breakpoint_put(struct process *proc, struct breakpoint *bkpt)
{
	bkpt->enabled--;
	if (bkpt->enabled == 0)
		disable_breakpoint(proc, bkpt);
}

static struct breakpoint *register_breakpoint(struct process *proc, addr_t addr)
{
	struct breakpoint *bkpt;
	addr_t fixed_addr;

	debug(1, "addr=0x%x", addr);

	fixed_addr = fixup_address(addr);
	bkpt = breakpoint_from_address(proc, fixed_addr);
	if (bkpt == NULL) {
		bkpt = calloc(1, sizeof(struct breakpoint));
		if (bkpt == NULL) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}
		dict_enter(proc->breakpoints, (void *)fixed_addr, bkpt);
		bkpt->addr = fixed_addr;
		bkpt->insn = breakpoint_instruction(addr);
	}
	breakpoint_get(proc, bkpt);

	return bkpt;
}

static void register_proc_breakpoints(struct process *proc)
{
	struct library_symbol *tmp;
	struct breakpoint *bkpt = NULL;

	tmp = proc->symbols;
	while (tmp) {
		bkpt = register_breakpoint(proc, tmp->enter_addr);
		bkpt->type = BKPT_ENTRY;
		bkpt->symbol = tmp;
		tmp = tmp->next;
	}
}

static void register_return_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	addr_t ret_addr;
	struct breakpoint *ret_bkpt;

	fn_return_address(proc, &ret_addr);
	ret_bkpt = register_breakpoint(proc, ret_addr);
	ret_bkpt->type = BKPT_RETURN;
	ret_bkpt->symbol = bkpt->symbol;
}

static void register_dl_debug_breakpoint(struct process *proc)
{
	addr_t addr = solib_dl_debug_address(proc);
	struct breakpoint *dl_bkpt;

	debug(3, "solib_dl_debug_address=0x%x", addr);
	dl_bkpt = register_breakpoint(proc, addr);
	dl_bkpt->type = BKPT_SOLIB;
}

#define set_pending_breakpoint(proc, bkpt) do { \
	proc->pending_breakpoint = bkpt; \
	} while (0)
#define clear_pending_breakpoint(proc) do { \
	proc->pending_breakpoint = NULL; \
	} while (0)

void bkpt_handle(struct process *proc, addr_t addr)
{
	struct breakpoint *bkpt = breakpoint_from_address(proc, addr);
	struct callback *cb = cb_get();

	debug(3, "pid=%d, addr=0x%x", proc->pid, addr);
	if (proc->pending_breakpoint != NULL) {
		/* re-enable pending breakpoint */
		enable_breakpoint(proc, proc->pending_breakpoint);
		clear_pending_breakpoint(proc);
	} else if (bkpt != NULL && bkpt->enabled) {
		switch (bkpt->type) {
		case BKPT_ENTRY:
			debug(1, "entry breakpoint for %s()", bkpt->symbol->name);
			register_return_breakpoint(proc, bkpt);
			fn_save_arg_data(proc);
			if (cb && cb->function.enter)
				cb->function.enter(proc, bkpt->symbol->name);
			break;
		case BKPT_RETURN:
			debug(1, "return breakpoint for %s()", bkpt->symbol->name);
			if (cb && cb->function.exit)
				cb->function.exit(proc, bkpt->symbol->name);
			breakpoint_put(proc, bkpt);
			fn_invalidate_arg_data(proc);
			break;
		case BKPT_SOLIB:
			solib_update_list(proc);
			register_proc_breakpoints(proc);
			break;
		default:
			error_exit("unknown breakpoint type");
		}
		/* temporarily disable breakpoint, so we can pass over it */
		if (bkpt->enabled) {
			disable_breakpoint(proc, bkpt);
			set_pending_breakpoint(proc, bkpt);
		}
		set_instruction_pointer(proc, addr);
	} else {
		msg_err("%s breakpoint at address 0x%x\n", bkpt ? "unexpected" : "unknown", addr);
		exit(EXIT_FAILURE);
	}
}

int bkpt_pending(struct process *proc)
{
	return (proc->pending_breakpoint != NULL);
}

void bkpt_init(struct process *proc)
{
#if 0 /* XXX not needed? */
	if (proc->breakpoints) {
		dict_apply_to_all(proc->breakpoints, free_bp_cb, NULL);
		dict_clear(proc->breakpoints);
		proc->breakpoints = NULL;
	}
#endif
	proc->breakpoints = dict_init(dict_key2hash_int, dict_key_cmp_int);
	register_dl_debug_breakpoint(proc);
	solib_update_list(proc);
	register_proc_breakpoints(proc);
}

static void disable_bkpt_cb(void *addr __unused, void *bkpt, void *proc)
{
	if (((struct breakpoint *)bkpt)->enabled) {
		disable_breakpoint((struct process *)proc, bkpt);
	}
}

void disable_all_breakpoints(struct process *proc)
{
	debug(1, "Disabling breakpoints for pid %d...", proc->pid);
	dict_apply_to_all(proc->breakpoints, disable_bkpt_cb, proc);
}
