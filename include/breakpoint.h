#ifndef FTK_BREAKPOINT_H
#define FTK_BREAKPOINT_H

#include "arch.h"
#include "process.h"
#include "target_mem.h"

struct breakpoint {
	addr_t addr;
	unsigned char orig_value[BREAKPOINT_LENGTH];
	int enabled;
	enum { BKPT_ENTRY, BKPT_RETURN, BKPT_SOLIB } type;
	struct library_symbol *symbol;
};

extern int bkpt_get_address(struct process *proc, addr_t *addr);
extern void bkpt_handle(struct process *proc, addr_t addr);
extern int bkpt_pending(struct process *proc);
extern void bkpt_init(struct process *proc);
extern void disable_all_breakpoints(struct process *proc);

#endif /* !FTK_BREAKPOINT_H */
