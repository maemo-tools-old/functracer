#ifndef FTK_CALLBACK_H
#define FTK_CALLBACK_H

#include "process.h"

struct callback {
	struct {
		void (*enter)(struct process *proc, const char *name);
		void (*exit)(struct process *proc, const char *name);
	} function;
	struct {
		void (*create)(struct process *proc);
		void (*exit)(struct process *proc, int exit_code);
		void (*kill)(struct process *proc, int signo);
		void (*signal)(struct process *proc, int signo);
		void (*interrupt)(struct process *proc);
	} process;
	struct {
		void (*enter)(struct process *proc, int sysno);
		void (*exit)(struct process *proc, int sysno);
	} syscall;
	struct {
		int (*match)(const char *name);
	} library;
};

extern void cb_init(void);
extern struct callback *cb_get(void);

#endif /* !FTK_CALLBACK_H */
