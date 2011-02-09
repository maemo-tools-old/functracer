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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "breakpoint.h"
#include "callback.h"
#include "debug.h"
#include "dict.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "solib.h"
#include "ssol.h"
#include "target_mem.h"
#include "context.h"

static void enable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	unsigned char safe_insn[MAX_INSN_SIZE];

	debug(1, "pid=%d, addr=0x%x", proc->pid, bkpt->addr);
	trace_mem_read(proc, bkpt->addr, bkpt->orig_insn.data, MAX_INSN_SIZE);
	if (ssol_prepare_bkpt(bkpt, &safe_insn) < 0) {
		msg_warn("Could not enable breakpoint for function \"%s\" at address %#x "
			 "(SSOL unsafe; see README for details)", bkpt->symbol ? bkpt->symbol : "<unknown>",
			 bkpt->addr);
		bkpt->enabled = 0;
		return;
	}
	trace_mem_write(proc, bkpt->ssol_addr, safe_insn, MAX_INSN_SIZE);
	trace_mem_write(proc, bkpt->addr, bkpt->insn->value, bkpt->insn->size);
	bkpt->enabled = 1;
}

static void disable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	if (!bkpt->enabled)
		return;
	trace_mem_write(proc, bkpt->addr, bkpt->orig_insn.data, MAX_INSN_SIZE);
}

static struct breakpoint *breakpoint_from_address(struct process *proc, addr_t addr)
{
	struct breakpoint *bkpt = dict_find_entry(proc->breakpoints, (void *)addr);
	return bkpt != NULL && !bkpt->enabled ? NULL : bkpt;
}

static void register_breakpoint_(struct process *proc, addr_t addr,
				 struct breakpoint *bkpt)
{
	dict_enter(proc->breakpoints, (void *)addr, bkpt);
	bkpt->refcnt++;
}

static void breakpoint_put(struct breakpoint *bkpt)
{
	if (--bkpt->refcnt == 0) {
		free(bkpt->symbol);
		free(bkpt);
	}
}

static struct breakpoint *register_breakpoint(struct process *proc, addr_t addr,
					      int type, const char* symname)
{
	struct breakpoint *bkpt;
	addr_t fixed_addr, ssol_addr;

	debug(1, "addr=0x%x", addr);

	fixed_addr = fixup_address(addr);
	bkpt = breakpoint_from_address(proc, fixed_addr);
	if (bkpt == NULL) {
		bkpt = calloc(1, sizeof(struct breakpoint));
		if (bkpt == NULL) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}
		if (symname) {
			bkpt->symbol = strdup(symname);
		}
		register_breakpoint_(proc, fixed_addr, bkpt);
		if (type == BKPT_RETURN || type == BKPT_SENTINEL) {
			ssol_addr = fixed_addr;
		} else {
			ssol_addr = ssol_new_slot(proc);
			register_breakpoint_(proc, ssol_addr, bkpt);
		}
		bkpt->addr = fixed_addr;
		bkpt->ssol_addr = ssol_addr;
		bkpt->type = type;
		bkpt->insn = breakpoint_instruction(addr);
	}
	if (type == BKPT_RETURN || type == BKPT_SENTINEL) {
		trace_mem_write(proc, bkpt->addr, bkpt->insn->value, bkpt->insn->size);
		bkpt->enabled = 1;
	} else
		enable_breakpoint(proc, bkpt);

	return bkpt;
}

static void register_entry_breakpoint(struct process *proc, const char *libname,
				      const char *symname, addr_t symaddr)
{
	struct breakpoint *bkpt, *bkpt2;

	if (context_match(symname) || plg_match(symname)) {
		bkpt = register_breakpoint(proc, symaddr, BKPT_ENTRY, symname);
		bkpt2 = register_breakpoint(proc, ssol_new_slot(proc), BKPT_SENTINEL, NULL);
		if (arguments.verbose)
			fprintf(stderr, "Registered breakpoint for function "
				"\"%s\" (%#x) from %s (PID %d)\n",
				bkpt->symbol, bkpt->addr, libname, proc->pid);
		debug(2, "entry breakpoint registered for \"%s\" at %#x, "
		      "SSOL %#x, sentinel %#x, PID %d", bkpt->symbol,
		      bkpt->addr, bkpt->ssol_addr, bkpt2->addr, proc->pid);
	}

}

static void register_dl_debug_breakpoint(struct process *proc)
{
	addr_t addr = solib_dl_debug_address(proc);

	if (addr == 0)
		return;
	debug(3, "solib_dl_debug_address=0x%x", addr);
	register_breakpoint(proc, addr, BKPT_SOLIB, NULL);
}

static void register_ssol_return_breakpoint(struct process *proc)
{
	register_breakpoint(proc, proc->ssol->first, BKPT_RETURN, NULL);
}

static int ssol_insn_size(struct process *proc, addr_t addr)
{
	int size;

	/* Skip first SSOL entry (reserved for return breakpoint). */
	if (addr <= proc->ssol->first ||
	    addr > proc->ssol->last + MAX_INSN_SIZE)
		return -1;

	size = addr - (addr / MAX_INSN_SIZE) * MAX_INSN_SIZE;
	if (size == 0)
		size = MAX_INSN_SIZE;
	return size;
}

void singlestep_handle(struct process *proc, addr_t addr)
{
	struct breakpoint *bkpt;
	int size = ssol_insn_size(proc, addr);

	debug(1, "pid=%d, addr=0x%x", proc->pid, addr);
	assert(size > 0 && size <= MAX_INSN_SIZE);
	bkpt = breakpoint_from_address(proc, addr - size);
	assert(bkpt != NULL);
	set_instruction_pointer(proc, bkpt->addr + size);
	/* Call SSOL post handler (if any). */
	if (bkpt->ssol_post_handler)
		bkpt->ssol_post_handler(proc, bkpt);
}

void singlestep_after_signal(struct process *proc)
{
	addr_t addr = bkpt_get_address(proc);
	struct breakpoint *bkpt = breakpoint_from_address(proc, addr);

	debug(2, "signal received while singlestepping (pid=%d, ssol=%#x)",
	      proc->pid, addr);
	assert(bkpt != NULL);
	if (proc->exiting) {
		/* If a process is exiting/detaching, we cannot point it to a
		 * SSOL address, so instead we point it to the original
		 * instruction. */
		addr = bkpt->addr;
	} else {
		/* Set instruction pointer to the sentinel breakpoint
		 * address. */
		addr += MAX_INSN_SIZE;
	}
	set_instruction_pointer(proc, addr);
	proc->singlestep = 0;
}

void bkpt_handle(struct process *proc, addr_t addr)
{
	struct breakpoint *bkpt = breakpoint_from_address(proc, addr);
	struct callback *cb = cb_get();
	char *symbol_name;

	debug(3, "pid=%d, addr=0x%x", proc->pid, addr);

	if (bkpt == NULL) {
		msg_err("unknown breakpoint at address 0x%x\n", addr);
		exit(EXIT_FAILURE);
	}

	switch (bkpt->type) {
	case BKPT_ENTRY:
		symbol_name = bkpt->symbol;
		debug(2, "entry breakpoint for %s()", symbol_name);
		/* Set instruction pointer to SSOL address before calling
		 * fn_callstack_push(), so that it can read the original
		 * instruction from it. */
		set_instruction_pointer(proc, bkpt->ssol_addr);
		if (fn_callstack_push(proc, symbol_name) == 0) {
			fn_set_return_address(proc, proc->ssol->first);
			if (cb && cb->function.enter)
				cb->function.enter(proc, symbol_name);
		}
		/* Call SSOL pre handler (if any). */
		if (bkpt->ssol_pre_handler)
			bkpt->ssol_pre_handler(proc, bkpt);
		proc->singlestep = 1;
		break;
	case BKPT_RETURN:
		symbol_name = fn_name(proc);
		debug(2, "return breakpoint for %s()", symbol_name);
		/* Fixup return address before calling function.exit()
		 * callback so that backtraces do not contain the SSOL
		 * address. */
		fn_get_return_address(proc, &addr);
		set_instruction_pointer(proc, addr);
		fn_callstack_restore(proc, 1);
		if (cb && cb->function.exit)
			cb->function.exit(proc, symbol_name);
		fn_callstack_restore(proc, 0);
		fn_callstack_pop(proc);
		break;
	case BKPT_SOLIB:
		debug(1, "solib breakpoint");
		solib_update_list(proc, register_entry_breakpoint);
		fn_do_return(proc);
		break;
	case BKPT_SENTINEL:
		/* A singlestep was interrupted by a signal. Restart it here.
		 */
		set_instruction_pointer(proc, addr - MAX_INSN_SIZE);
		proc->singlestep = 1;
		break;
	default:
		error_exit("unknown breakpoint type");
	}
}

void bkpt_init(struct process *proc)
{
	if (proc->parent == NULL) {
		proc->breakpoints = dict_init(dict_key2hash_int, dict_key_cmp_int);
		ssol_init(proc);
		register_ssol_return_breakpoint(proc);
		register_dl_debug_breakpoint(proc);
		solib_update_list(proc, register_entry_breakpoint);
	} else {
		proc->breakpoints = proc->parent->breakpoints;
		proc->solib_list = proc->parent->solib_list;
		proc->ssol = proc->parent->ssol;
	}
}

static void disable_bkpt_cb(void *addr __unused, void *bkpt, void *proc)
{
	disable_breakpoint((struct process *)proc, bkpt);
}

void disable_all_breakpoints(struct process *proc)
{
	if (proc->breakpoints == NULL)
		return;
	debug(1, "Disabling breakpoints for pid %d...", proc->pid);
	dict_apply_to_all(proc->breakpoints, disable_bkpt_cb, proc);
}

static void free_bkpt_cb(void *addr __unused, void *bkpt, void *proc __unused)
{
	breakpoint_put((struct breakpoint *)bkpt);
}

static void free_all_breakpoints(struct process *proc)
{
	if (proc->breakpoints == NULL)
		return;
	debug(1, "Freeing breakpoints for pid %d...", proc->pid);
	dict_apply_to_all(proc->breakpoints, free_bkpt_cb, proc);
	dict_clear(proc->breakpoints);
	proc->breakpoints = NULL;
}

void bkpt_finish(struct process *proc)
{
	if (proc->parent != NULL)
		return;
	free_all_solibs(proc);
	ssol_finish(proc);
	free_all_breakpoints(proc);
}
