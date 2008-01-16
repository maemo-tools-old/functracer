#include <linux/ptrace.h>

#include "sysdeps.h"

int get_instruction_pointer(struct process *proc, addr_t *addr)
{
	*addr = trace_user_readw(proc, 4 * EIP);

	return 0;
}

int set_instruction_pointer(struct process *proc, addr_t addr)
{
	trace_user_writew(proc, 4 * EIP, (long)addr);

	return 0;
}
