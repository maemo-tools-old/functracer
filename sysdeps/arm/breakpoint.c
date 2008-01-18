#include <linux/ptrace.h>

#include "breakpoint.h"
#include "debug.h"

#define off_pc 60

addr_t bkpt_get_address(struct process *proc)
{
	return trace_user_readw(proc, off_pc);
}

void set_instruction_pointer(struct process *proc, addr_t addr)
{
	trace_user_writew(proc, off_pc, (long)addr);
}

struct bkpt_insn *breakpoint_instruction(addr_t addr)
{
	/* ARM mode breakpoint */
	static struct bkpt_insn arm_insn = {
		.value = { 0xf0, 0x01, 0xf0, 0xe7 },
		.size = 4,
	};
	/* Thumb mode breakpoint */
	static struct bkpt_insn thumb_insn = {
		.value = { 0x01, 0xde },
		.size = 2,
	};

	return (addr & 1) ? &thumb_insn : &arm_insn;
}

addr_t fixup_address(addr_t addr)
{
	debug(3, "addr=0x%x, thumb=%d", addr, addr & 1);
        /* fixup address for thumb instruction */
	return (addr & 1) ? (addr & (addr_t)~1) : addr;
}
