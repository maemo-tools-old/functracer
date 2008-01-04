#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include <asm/unistd.h>		/* for syscall numbers */
#include <linux/futex.h>	/* for futex() tracing */

#include "debug.h"
#include "elf.h"
#include "event.h"
#include "process.h"
#include "ptrace.h"
#include "syscall.h"
#include "options.h"

#if 0
void print_status(pid_t pid, int status)
{
	if (WIFEXITED(status))
		fprintf(stderr, "\nProcess %d exited with status %d", pid,
			WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		fprintf(stderr, "\nProcess %d killed by signal %d", pid,
			WTERMSIG(status));
	if (WIFSTOPPED(status))
		fprintf(stderr, "\nProcess %d stopped by signal %d %s",
			pid,
			WSTOPSIG(status) & 0x80 ? WSTOPSIG(status) & ~0x80
			: WSTOPSIG(status),
			WSTOPSIG(status) & 0x80 ? "(syscall)" : status >>
			16 ? "(extended event)" : "(breakpoint)");
	if (WIFCONTINUED(status))
		fprintf(stderr, "\nProcess %d continued by SIGCONT", pid);
	fprintf(stderr, ", status = 0x%x\n", status);
}

void print_symbols(struct process *proc)
{
	struct library_symbol *tmp = proc->list_of_symbols;

	printf("symbols for PID %d:\n", proc->pid);
	while (tmp) {
		printf("\tname = %s, addr = %p\n", tmp->name, tmp->enter_addr);
		tmp = tmp->next;
	}
}
#endif

pid_t execute_program(char *filename, char *argv[])
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		error_exit("fork");
	} else if (!pid) {	/* child */
		trace_me();
		execvp(filename, argv);
		error_exit("could not execute \"%s\"", filename);
	}

	return pid;
}

int main(int argc, char *argv[])
{
	int prog_index;
	struct arguments arguments;
	
	memset(&arguments, 0, sizeof(struct arguments));

	process_options(argc, argv, &prog_index, &arguments);
	
	execute_program(argv[prog_index], argv + prog_index);

	return event_loop();
}
