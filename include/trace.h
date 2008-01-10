#ifndef FTK_TRACE_H
#define FTK_TRACE_H

#include "process.h"

struct trace_cb {
	struct {
		void (*create)(struct process *proc);
		void (*exit)(struct process *proc, int exit_code);
		void (*kill)(struct process *proc, int signo);
		void (*signal)(struct process *proc, int signo);
	} process;
	struct {
		void (*enter)(struct process *proc, int sysno);
		void (*exit)(struct process *proc, int sysno);
	} syscall;
};

extern int trace_main_loop(void);
extern int trace_attach(pid_t pid);
extern int trace_execute(char *filename, char *argv[]);
extern void trace_finish(void);
extern void trace_register_callbacks(struct trace_cb *tcb);
extern void trace_callbacks_init(void);

#endif /* !FTK_TRACE_H */
