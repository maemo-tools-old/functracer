#ifndef FTK_BREAKPOINT_H
#define FTK_BREAKPOINT_H

#include "process.h"
#include "target_mem.h"

/* maximum breakpoint instruction size for all supported architectures */
#define BREAKPOINT_LENGTH	4

struct bkpt_insn {
	unsigned char value[BREAKPOINT_LENGTH];
	size_t size;
};

struct breakpoint {
	addr_t addr;
	struct bkpt_insn *insn;
	unsigned char orig_insn[BREAKPOINT_LENGTH];
	int enabled;
	enum { BKPT_ENTRY, BKPT_RETURN, BKPT_SOLIB } type;
	struct library_symbol *symbol;
};

extern void set_instruction_pointer(struct process *proc, addr_t addr);
extern addr_t bkpt_get_address(struct process *proc);
extern struct bkpt_insn *breakpoint_instruction(addr_t addr);
extern addr_t fixup_address(addr_t addr);
extern void bkpt_handle(struct process *proc, addr_t addr);
extern int bkpt_pending(struct process *proc);
extern void bkpt_init(struct process *proc);

#endif /* !FTK_BREAKPOINT_H */
