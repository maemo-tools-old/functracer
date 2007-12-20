#ifndef TT_PTRACE_H
#define TT_PTRACE_H

#include <sys/types.h>
#include "process.h"

extern void continue_process(struct process *proc);
extern void trace_set_options(struct process *proc);
extern void trace_me(void);
extern int fork_event(struct process *proc, int event, pid_t *new_pid);

#endif /* TT_PTRACE_H */
