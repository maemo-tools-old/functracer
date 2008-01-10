#ifndef TT_PTRACE_H
#define TT_PTRACE_H

#include "process.h"

extern void trace_mem_read(struct process *proc, void *addr, void *buf, size_t count);
extern void trace_mem_write(struct process *proc, void *addr, void *buf, size_t count);
extern void trace_user_read(struct process *proc, int off, long *val);
extern void trace_user_write(struct process *proc, int off, long val);

#endif /* TT_PTRACE_H */
