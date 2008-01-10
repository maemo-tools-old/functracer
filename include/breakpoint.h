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

extern int bkpt_get_address(struct process *proc, void **addr);
extern void bkpt_handle(struct process *proc, void *addr);
extern int bkpt_pending(struct process *proc);
extern void bkpt_register_callbacks(struct breakpoint_cb *bcb);
extern void bkpt_init(struct process *proc);

#endif /* ft_BREAKPOINT_H */
