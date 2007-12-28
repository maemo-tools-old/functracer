#ifndef TT_PTRACE_H
#define TT_PTRACE_H

#include <sys/types.h>
#include "process.h"

extern void continue_process(struct process *proc);
extern void trace_set_options(struct process *proc);
extern void trace_me(void);
extern int is_fork(struct process *proc, int event);
extern void get_fork_pid(struct process *proc, pid_t *new_pid);

#endif /* TT_PTRACE_H */
