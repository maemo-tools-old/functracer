#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "debug.h"
#include "process.h"

static struct process *list_of_processes = NULL;

struct process *get_list_of_processes(void)
{
	return list_of_processes;
}

struct process *process_from_pid(pid_t pid)
{
	struct process *tmp;

	tmp = list_of_processes;
	while (tmp && tmp->pid != pid)
		tmp = tmp->next;
	return tmp;
}

static char *name_from_pid(pid_t pid)
{
	char proc_exe[1024];

	if (kill(pid, 0) == 0) {
		snprintf(proc_exe, 1024, "/proc/%d/exe", pid);
		if (access(proc_exe, F_OK) < 0)
			error_exit("access");
		return strdup(proc_exe);
	}
	return NULL;
}

struct process *add_process(pid_t pid)
{
	struct process *tmp;

	tmp = calloc(1, sizeof(struct process));
	if (!tmp)
		error_exit("malloc");
	tmp->pid = pid;
	tmp->filename = name_from_pid(pid);
	tmp->next = list_of_processes;
	list_of_processes = tmp;

	debug(1, "Adding PID %d, filename = \"%s\"", pid, tmp->filename);

	return tmp;
}

void remove_process(struct process *proc)
{
	struct process *tmp;

	debug(1, "Removing PID %d", proc->pid);

	if (list_of_processes->pid == proc->pid) {
		tmp = list_of_processes;
		list_of_processes = list_of_processes->next;
		if (tmp->filename)
			free(tmp->filename);
		free(tmp);
		return;
	}
	tmp = list_of_processes;
	while (tmp->next) {
		if (tmp->next->pid == proc->pid) {
			struct process *tmp2 = tmp->next;
			tmp->next = tmp->next->next;
			free(tmp2);
			return;
		}
		tmp = tmp->next;
	}
}