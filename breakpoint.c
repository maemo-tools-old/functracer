#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "breakpoint.h"
#include "debug.h"
#include "dict.h"
#include "process.h"
#include "ptrace.h"
#include "sysdeps.h"

static void enable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	unsigned char bpkt_insn[] = BREAKPOINT_VALUE;

	debug(1, "enable_breakpoint(%d,%p)", proc->pid, bkpt->addr);

	trace_mem_io(proc, bkpt->addr, bkpt->orig_value, BREAKPOINT_LENGTH, 0);
	trace_mem_io(proc, bkpt->addr, bpkt_insn, BREAKPOINT_LENGTH, 1);
}

static void disable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	debug(1, "disable_breakpoint(%d,%p)", proc->pid, bkpt->addr);

	trace_mem_io(proc, bkpt->addr, bkpt->orig_value, BREAKPOINT_LENGTH, 1);
}

static void init_breakpoints(struct process *proc)
{
	debug(3, "init_breakpoints(PID %d)", proc->pid);
#if 0 /* XXX not needed? */
	if (proc->breakpoints) {
		dict_apply_to_all(proc->breakpoints, free_bp_cb, NULL);
		dict_clear(proc->breakpoints);
		proc->breakpoints = NULL;
	}
#endif
	proc->breakpoints = dict_init(dict_key2hash_int, dict_key_cmp_int);
}

static struct breakpoint *register_breakpoint(struct process *proc, void *addr)
{
	struct breakpoint *bkpt;

	debug(1, "addr=%p", addr);

	if (proc->breakpoints == NULL) {
		init_breakpoints(proc);
	}
	bkpt = dict_find_entry(proc->breakpoints, addr);
	if (bkpt == NULL) {
		bkpt = calloc(1, sizeof(struct breakpoint));
		if (bkpt == NULL) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}
		dict_enter(proc->breakpoints, addr, bkpt);
		bkpt->addr = addr;
	}
	bkpt->enabled++;
	if (bkpt->enabled == 1)
		enable_breakpoint(proc, bkpt);

	return bkpt;
}

int get_breakpoint_address(struct process *proc, void **addr)
{
	int ret;

	if ((ret = get_instruction_pointer(proc, addr)) < 0)
		return ret;

	return 0;
}

int register_alloc_breakpoints(struct process *proc)
{
	struct library_symbol *tmp;
	struct breakpoint *bkpt= NULL;

	tmp = proc->symbols;
	while (tmp) {
		if (strcmp(tmp->name, "malloc") == 0) {
			bkpt = register_breakpoint(proc, tmp->enter_addr);
			bkpt->symbol = tmp;
			break;
		}
		tmp = tmp->next;
	}

	return 0;
}

static struct breakpoint *breakpoint_from_address(struct process *proc, void *addr)
{
	return dict_find_entry(proc->breakpoints, addr);
}

void process_breakpoint(struct process *proc, void *addr)
{
	struct breakpoint *bkpt = breakpoint_from_address(proc, addr);

	if (proc->pending_breakpoint != NULL) {
		enable_breakpoint(proc, proc->pending_breakpoint);
		proc->pending_breakpoint = NULL;
	} else if (bkpt != NULL) {
		debug(1, "breakpoint for function \"%s\"", bkpt->symbol->name);
		disable_breakpoint(proc, bkpt);
		set_instruction_pointer(proc, addr);
		proc->pending_breakpoint = bkpt;
	} else {
		error_exit("unknown breakpoint at address %p\n", addr);
	}
}

void enable_pending_breakpoint(struct process *proc)
{
	enable_breakpoint(proc, proc->pending_breakpoint);
}

int pending_breakpoint(struct process *proc)
{
	return (proc->pending_breakpoint != NULL);
}
