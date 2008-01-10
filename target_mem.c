#include <stdio.h>
//#include <stdlib.h>
#include <sys/ptrace.h>
//#include <linux/ptrace.h>
#include <errno.h>

#include "debug.h"
#include "target_mem.h"

static void trace_mem_io(struct process *proc, void *addr, void *buf, size_t count, int write)
{
	long w;
	unsigned int i, j;
	unsigned char *buf_bytes = buf;
	unsigned char *w_bytes = (unsigned char *)&w;

	debug(3, "trace_mem_io(pid=%d, addr=%p, buf=%p, count=%d, write=%d", proc->pid, addr, buf, count, write);
	for (i = 0; i < 1 + ((count - 1) / sizeof(long)); i++) {
		errno = 0;
		w = ptrace(PTRACE_PEEKTEXT, proc->pid, addr + i * sizeof(long), 0);
		if (w == -1 && errno) {
			error_exit("trace_mem_io (read): ptrace");
		}
		debug(4, "trace_mem_io (read): w = 0x%08lx", w);
		for (j = 0; j < sizeof(long) && i * sizeof(long) + j < count; j++) {
			if (write)
				w_bytes[j] = buf_bytes[i * sizeof(long) + j];
			else
				buf_bytes[i * sizeof(long) + j] = w_bytes[j];
		}
		if (write) {
			errno = 0;
			if (ptrace(PTRACE_POKETEXT, proc->pid, addr + i * sizeof(long), w) == -1 && errno) {
				error_exit("trace_mem_io (write): ptrace");
			}
			debug(4, "trace_mem_io (write): w = 0x%08lx", w);
		}
	}
}

void trace_mem_read(struct process *proc, void *addr, void *buf, size_t count)
{
	trace_mem_io(proc, addr, buf, count, 0);
}

void trace_mem_write(struct process *proc, void *addr, void *buf, size_t count)
{
	trace_mem_io(proc, addr, buf, count, 1);
}

void trace_user_read(struct process *proc, int off, long *val)
{
	errno = 0;
	*val = ptrace(PTRACE_PEEKUSER, proc->pid, off, 0);
	if (*val == -1 && errno)
		error_exit("trace_user_read: ptrace");
}

void trace_user_write(struct process *proc, int off, long val)
{
	errno = 0;
	if (ptrace(PTRACE_POKEUSER, proc->pid, off, val) == -1 && errno)
		error_exit("trace_user_write: ptrace");
}

void trace_getregs(struct process *proc, void *regs)
{
	errno = 0;
	if (ptrace(PTRACE_GETREGS, proc->pid, 0, regs) == -1 && errno)
		error_exit("trace_getregs: ptrace");
}
