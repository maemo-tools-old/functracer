#include <linux/ptrace.h>

#include "breakpoint.h"
#include "debug.h"

addr_t bkpt_get_address(struct process *proc)
{
	addr_t addr;

	debug(3, "pid=%d", proc->pid);
	addr = trace_user_readw(proc, 4 * EIP);
	/* EIP is always incremented by 1 after hitting a breakpoint, so
 	 * decrement it to get the actual breakpoint address. */
	addr -= 1;

	return addr;
}

void set_instruction_pointer(struct process *proc, addr_t addr)
{
	trace_user_writew(proc, 4 * EIP, (long)addr);
}

struct bkpt_insn *breakpoint_instruction(addr_t addr)
{
	static struct bkpt_insn insn = {
		.value = { 0xcc },
		.size = 1,
	};
	return &insn;
}

addr_t fixup_address(addr_t addr)
{
	/* no fixup needed for x86 */
	return addr;
}
