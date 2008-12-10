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

static void enable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	long orig_insn;

	debug(1, "pid=%d, addr=0x%x", proc->pid, bkpt->addr);
	orig_insn = trace_mem_readw(proc, bkpt->addr);
	trace_mem_writew(proc, bkpt->ssol_addr, orig_insn);
	trace_mem_write(proc, bkpt->addr, bkpt->insn->value, bkpt->insn->size);
}

static void disable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	long orig_insn = trace_mem_readw(proc, bkpt->ssol_addr);
	trace_mem_writew(proc, bkpt->addr, orig_insn);
}

static struct breakpoint *breakpoint_from_address(struct process *proc, addr_t addr)
{
	return dict_find_entry(proc->breakpoints, (void *)addr);
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
					      int type)
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
	enable_breakpoint(proc, bkpt);

	return bkpt;
}

static void register_entry_breakpoint(struct process *proc, const char *libname,
				      const char *symname, addr_t symaddr)
{
	struct breakpoint *bkpt, *bkpt2;

	if (plg_match(symname)) {
		bkpt = register_breakpoint(proc, symaddr, BKPT_ENTRY);
		bkpt->symbol = strdup(symname);
		bkpt2 = register_breakpoint(proc, ssol_new_slot(proc), BKPT_SENTINEL);
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

	debug(3, "solib_dl_debug_address=0x%x", addr);
	register_breakpoint(proc, addr, BKPT_SOLIB);
}

static void register_ssol_return_breakpoint(struct process *proc)
{
	register_breakpoint(proc, proc->ssol->first, BKPT_RETURN);
}

static int ssol_insn_size(struct process *proc, addr_t addr)
{
	int size;

	/* Skip first SSOL entry (reserved for return breakpoint). */
	if (addr <= proc->ssol->first ||
	    addr > proc->ssol->last + sizeof(long))
		return -1;

	size = addr - (addr / sizeof(long)) * sizeof(long);
	if (size == 0)
		size = sizeof(long);
	return size;
}

void singlestep_handle(struct process *proc, addr_t addr)
{
	struct breakpoint *bkpt;
	int size = ssol_insn_size(proc, addr);

	debug(1, "pid=%d, addr=0x%x", proc->pid, addr);
	assert(size > 0 && size <= sizeof(long));
	bkpt = breakpoint_from_address(proc, addr - size);
	assert(bkpt != NULL);
	set_instruction_pointer(proc, bkpt->addr + size);
}

void singlestep_after_signal(struct process *proc)
{
	addr_t addr = bkpt_get_address(proc);

	debug(2, "signal received while singlestepping (pid=%d, ssol=%#x)",
	      proc->pid, addr);
	if (proc->exiting) {
		/* If a process is exiting/detaching, we cannot point it to a
		 * SSOL address, so instead we point it to the original
		 * instruction. */
		struct breakpoint *bkpt = breakpoint_from_address(proc, addr);
		addr = bkpt->addr;
	} else {
		addr += sizeof(long);
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
		set_instruction_pointer(proc, addr - sizeof(long));
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
