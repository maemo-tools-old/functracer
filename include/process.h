#ifndef TT_PROCESS_H
#define TT_PROCESS_H

#include <sys/types.h>
#include "library.h"

struct process {
	pid_t pid;
	char *filename;
	struct dict *breakpoints;
	struct library_symbol *symbols;
	struct breakpoint *pending_breakpoint;

	int is_ptraced;
	int in_syscall;
	short e_machine;
	struct process *next;
};

extern struct process *process_from_pid(pid_t pid);
extern struct process *add_process(pid_t pid);
extern void remove_process(struct process *proc);

#endif /* TT_PROCESS_H */
