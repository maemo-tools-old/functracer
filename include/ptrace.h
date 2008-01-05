#ifndef TT_PTRACE_H
#define TT_PTRACE_H

#include <sys/types.h>
#include "process.h"

extern void continue_process(struct process *proc);
extern void trace_set_options(struct process *proc);
extern void trace_me(void);
extern int is_fork(struct process *proc, int event);
extern void get_fork_pid(struct process *proc, pid_t *new_pid);
extern void trace_mem_read(struct process *proc, void *addr, void *buf, size_t count);
extern void trace_mem_write(struct process *proc, void *addr, void *buf, size_t count);
extern void trace_user_read(struct process *proc, int off, long *val);
extern void trace_user_write(struct process *proc, int off, long val);

#endif /* TT_PTRACE_H */
