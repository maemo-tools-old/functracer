#ifndef TT_PROCESS_H
#define TT_PROCESS_H

#include <sys/types.h>
#include "library.h"

struct process {
	pid_t pid;
	char *filename;
	int is_ptraced;
	int in_syscall;
	struct library_symbol *list_of_symbols;
	struct breakpoint *cur_bkpt;

	short e_machine;
	struct process *next;
};

extern struct process *pid2proc(pid_t pid);
extern struct process *add_proc(pid_t pid);
extern void remove_proc(struct process *proc);

#endif /* TT_PROCESS_H */
