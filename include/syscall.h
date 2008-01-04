#ifndef TT_SYSCALL_H
#define TT_SYSCALL_H

#include "process.h"

extern int get_syscall_nr(struct process *proc, int *nr);
extern long get_syscall_arg(struct process *proc, int arg_num);

#endif /* TT_SYSCALL_H */
