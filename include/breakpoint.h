#ifndef ft_BREAKPOINT_H
#define ft_BREAKPOINT_H

#include "process.h"

#ifdef __i386__

#define BREAKPOINT_VALUE {0xcc}
#define BREAKPOINT_LENGTH 1
#define DECR_PC_AFTER_BREAK 1

#endif

struct breakpoint {
	void *addr;
	unsigned char orig_value[BREAKPOINT_LENGTH];
	int enabled;
	struct library_symbol *symbol;
};

extern int get_breakpoint_address(struct process *proc, void **addr);
extern int register_alloc_breakpoints(struct process *proc);
extern void process_breakpoint(struct process *proc, void *addr);
extern int pending_breakpoint(struct process *proc);

#endif /* ft_BREAKPOINT_H */
