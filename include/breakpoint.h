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
	enum { BKPT_ENTRY, BKPT_RETURN, BKPT_SOLIB } type;
	struct library_symbol *symbol;
};

struct breakpoint_cb {
	struct {
		void (*enter)(struct process *proc, const char *name);
		void (*exit)(struct process *proc, const char *name);
		int (*match)(struct process *proc, const char *name);
	} function;
};

extern int get_breakpoint_address(struct process *proc, void **addr);
extern int register_alloc_breakpoints(struct process *proc);
extern void handle_breakpoint(struct process *proc, void *addr);
extern int pending_breakpoint(struct process *proc);
extern void breakpoint_register_callbacks(struct breakpoint_cb *bcb);

#endif /* ft_BREAKPOINT_H */
