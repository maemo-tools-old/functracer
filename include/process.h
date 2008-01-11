#ifndef TT_PROCESS_H
#define TT_PROCESS_H

#include <sys/types.h>

struct dict;
struct breakpoint;
struct library_symbol;
struct rp_data;
struct solib_list;

struct callstack {
	void *fn_arg_data;
	struct callstack *next;
};

struct process {
	pid_t pid;
	char *filename;
	struct dict *breakpoints;
	struct library_symbol *symbols;
	struct solib_list *solib_list;
	struct breakpoint *pending_breakpoint;
	struct rp_data *rp_data;
	struct callstack *callstack;
	int in_syscall;
	struct process *next;
};

extern struct process *process_from_pid(pid_t pid);
extern struct process *add_process(pid_t pid);
extern void remove_process(struct process *proc);

#endif /* TT_PROCESS_H */
