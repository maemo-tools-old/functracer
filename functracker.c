#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include <asm/unistd.h> /* for syscall numbers */
#include <linux/futex.h> /* for futex() tracing */

#include "process.h"
#include "ptrace.h"
#include "syscall.h"
#include "elf.h"
#include "debug.h"

void print_status(pid_t pid, int status)
{
	if (WIFEXITED(status))
		fprintf(stderr, "\nProcess %d exited with status %d", pid, WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		fprintf(stderr, "\nProcess %d killed by signal %d", pid, WTERMSIG(status));
	if (WIFSTOPPED(status))
		fprintf(stderr, "\nProcess %d stopped by signal %d %s", pid, WSTOPSIG(status) & 0x80 ? WSTOPSIG(status) & ~0x80 : WSTOPSIG(status), WSTOPSIG(status) & 0x80 ? "(syscall)" : status >> 16 ? "(extended event)" : "(breakpoint)");
	if (WIFCONTINUED(status))
		fprintf(stderr, "\nProcess %d continued by SIGCONT", pid);
	fprintf(stderr, ", status = 0x%x\n", status);
}

#if 0
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

void wait_for_something(void)
{
	pid_t pid;
	int status;
	struct process *proc;

	pid = waitpid(-1, &status, __WALL);
	if (pid == -1) {
		if (errno == ECHILD) {
			debug(1, "No more children");
			exit(0);
		} else if (errno == EINTR) {
			fprintf(stderr, "wait received EINTR ?\n");
		}		
		perror("waitpid");
		exit(1);
	}
	proc = pid2proc(pid);
	if (!proc) {
		proc = add_proc(pid);
//		proc->list_of_symbols = read_elf(proc);
//		insert_function_bp(proc, "printf");
//		print_symbols(proc);
		trace_set_options(proc);
		continue_process(proc);
		return;
	}
	print_status(pid, status);
	if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
		pid_t new_pid;

		if (fork_event(proc, status >> 16, &new_pid) == 0 && new_pid) {
			continue_process(proc);
			return;
		}
	}
	if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80)) {
		static int in_syscall = 0;
		int nr, ret;

		ret = get_syscall_nr(proc, &nr);
		fprintf(stderr, "\nSyscall: ret = %d, nr = %d\n", ret, nr);
		switch (ret) {
		case 1:
			if (nr == __NR_futex) {
				int *uaddr, op, val;

				uaddr = (int *)get_syscall_arg(proc, 0);
				op = get_syscall_arg(proc, 1);
				val = get_syscall_arg(proc, 2);

				if (in_syscall)
					fprintf(stderr, " <unfinished ...>\n");
#define futex_op_str(op)	(op == 0 ? "FUTEX_WAIT" : op == 1 ? "FUTEX_WAKE" : "<other>")
				fprintf(stderr, "%d futex(%p, %s, %d", proc->pid, uaddr, futex_op_str(op), val);
				if (op == FUTEX_WAIT) {
					void *timeout = (void *)get_syscall_arg(proc, 3);
					if (timeout)
						fprintf(stderr, ", %p", timeout);
					else
						fprintf(stderr, ", NULL");
				}
				in_syscall = 1;
			}
			break;
		case 2:
			if (nr == __NR_futex) {
				int ret = get_syscall_arg(proc, -1);

				if (!in_syscall)
					fprintf(stderr, "%d <... futex resumed> ", proc->pid);
				fprintf(stderr, ") = %d\n", ret);
				in_syscall = 0;
			}
			break;
		}
	}
	if (WIFEXITED(status)) {
		if (!(kill(proc->pid, 0) < 0 && errno == ESRCH))
			debug(1, "warning: dangling process %d", pid);
		remove_proc(proc);
		return;
	}
	continue_process(proc);
}

void execute_program(char *filename, char *argv[])
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (!pid) {	/* child */
		trace_me();
		execvp(filename, argv);
		fprintf(stderr, "Can't execute \"%s\": %s\n", filename,	strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <program> <args>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	execute_program(argv[1], argv + 1);
	while (1)
		wait_for_something();

	return 0;
}
