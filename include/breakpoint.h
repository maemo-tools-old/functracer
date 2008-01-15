#ifndef FTK_BREAKPOINT_H
#define FTK_BREAKPOINT_H

#include "process.h"
#include "arch.h"

struct breakpoint {
	void *addr;
	unsigned char orig_value[BREAKPOINT_LENGTH];
	int enabled;
	enum { BKPT_ENTRY, BKPT_RETURN, BKPT_SOLIB } type;
	struct library_symbol *symbol;
};

extern int bkpt_get_address(struct process *proc, void **addr);
extern void bkpt_handle(struct process *proc, void *addr);
extern int bkpt_pending(struct process *proc);
extern void bkpt_init(struct process *proc);

#endif /* !FTK_BREAKPOINT_H */
