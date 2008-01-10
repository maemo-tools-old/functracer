#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "breakpoint.h"
#include "debug.h"
#include "dict.h"
#include "function.h"
#include "process.h"
#include "report.h"
#include "solib.h"
#include "sysdeps.h"
#include "target_mem.h"
#include "util.h"

static struct breakpoint_cb *current_bcb = NULL;

static void enable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	unsigned char bpkt_insn[] = BREAKPOINT_VALUE;

	debug(1, "pid=%d, addr=%p", proc->pid, bkpt->addr);

	trace_mem_read(proc, bkpt->addr, bkpt->orig_value, BREAKPOINT_LENGTH);
	trace_mem_write(proc, bkpt->addr, bpkt_insn, BREAKPOINT_LENGTH);
}

static void disable_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	debug(1, "pid=%d, addr=%p, orig_value=0x%x", proc->pid, bkpt->addr, bkpt->orig_value[0]);

	trace_mem_write(proc, bkpt->addr, bkpt->orig_value, BREAKPOINT_LENGTH);
}

static struct breakpoint *register_breakpoint(struct process *proc, void *addr)
{
	struct breakpoint *bkpt;

	debug(1, "addr=%p", addr);

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

int bkpt_get_address(struct process *proc, void **addr)
{
	int ret;

	if ((ret = get_instruction_pointer(proc, addr)) < 0)
		return ret;
	*addr -= DECR_PC_AFTER_BREAK;

	return 0;
}

int register_interesting_breakpoints(struct process *proc)
{
	struct library_symbol *tmp;
	struct breakpoint *bkpt = NULL;
	struct breakpoint_cb *bcb = current_bcb;

	tmp = proc->symbols;
	while (tmp) {
		if (bcb && bcb->function.match
		    && bcb->function.match(proc, tmp->name)) {
			bkpt = register_breakpoint(proc, tmp->enter_addr);
			bkpt->type = BKPT_ENTRY;
			bkpt->symbol = tmp;
			break;
		}
		tmp = tmp->next;
	}

	return 0;
}

void register_dl_debug_breakpoint(struct process *proc)
{
	printf("dl addr = %p\n", solib_dl_debug_address(proc));
}

static void register_return_breakpoint(struct process *proc, struct breakpoint *bkpt)
{
	void *ret_addr;
	struct breakpoint *ret_bkpt;

	fn_return_address(proc, &ret_addr);
	ret_bkpt = register_breakpoint(proc, ret_addr);
	ret_bkpt->type = BKPT_RETURN;
	ret_bkpt->symbol = bkpt->symbol;
}

static struct breakpoint *breakpoint_from_address(struct process *proc, void *addr)
{
	return dict_find_entry(proc->breakpoints, addr);
}

#define set_pending_breakpoint(proc, bkpt) do { \
	proc->pending_breakpoint = bkpt; \
	} while (0)
#define clear_pending_breakpoint(proc) do { \
	proc->pending_breakpoint = NULL; \
	} while (0)

void bkpt_handle(struct process *proc, void *addr)
{
	struct breakpoint *bkpt = breakpoint_from_address(proc, addr);
	struct breakpoint_cb *bcb = current_bcb;

	if (proc->pending_breakpoint != NULL) {
		/* re-enable pending breakpoint */
		enable_breakpoint(proc, proc->pending_breakpoint);
		clear_pending_breakpoint(proc);
	} else if (bkpt != NULL) {
		debug(1, "%s breakpoint for %s()",
		      bkpt->type == BKPT_ENTRY ? "entry" : "return",
		      bkpt->symbol->name);
		switch (bkpt->type) {
		case BKPT_ENTRY:
			register_return_breakpoint(proc, bkpt);
			set_pending_breakpoint(proc, bkpt);
			fn_save_arg_data(proc);
			if (bcb && bcb->function.enter)
				bcb->function.enter(proc, bkpt->symbol->name);
			break;
		case BKPT_RETURN:
			if (bcb && bcb->function.exit)
				bcb->function.exit(proc, bkpt->symbol->name);
			fn_invalidate_arg_data(proc);
			break;
		case BKPT_SOLIB:
			set_pending_breakpoint(proc, bkpt);
			solib_update_list(proc);
			break;
		default:
			error_exit("unknown breakpoint type");
		}
		/* temporarily disable breakpoint, so we can pass over it */
		disable_breakpoint(proc, bkpt);
		set_instruction_pointer(proc, addr);
	} else {
		error_exit("unknown breakpoint at address %p\n", addr);
	}
}

int bkpt_pending(struct process *proc)
{
	return (proc->pending_breakpoint != NULL);
}

void bkpt_register_callbacks(struct breakpoint_cb *bcb)
{
	if (current_bcb)
		free(current_bcb);
	current_bcb = xmalloc(sizeof(struct breakpoint_cb));
	memcpy(current_bcb, bcb, sizeof(struct breakpoint_cb));
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
	
}
