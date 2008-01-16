#ifndef TT_PTRACE_H
#define TT_PTRACE_H

#include <stdint.h>

#include "process.h"

/*typedef void *addr_t;*/
typedef uintptr_t addr_t;

extern long trace_mem_readw(struct process *proc, addr_t addr);
extern void trace_mem_writew(struct process *proc, addr_t addr, long w);
extern long trace_user_readw(struct process *proc, long offset);
extern void trace_user_writew(struct process *proc, long offset, long w);
extern void trace_mem_read(struct process *proc, addr_t addr, void *buf, size_t count);
extern void trace_mem_write(struct process *proc, addr_t addr, void *buf, size_t count);
extern void trace_getregs(struct process *proc, void *regs);

#endif /* TT_PTRACE_H */
