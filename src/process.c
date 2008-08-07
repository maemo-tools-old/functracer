/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "debug.h"
#include "options.h"
#include "process.h"

#define BUF_SIZE	4096
#define FIELD_NAME	"Tgid:"
#define FIELD_SIZE	(sizeof(FIELD_NAME) - 1)

static struct process *list_of_processes = NULL;

static pid_t ft_waitpid(pid_t pid, int *status, int options)
{
	pid_t pid2;

	errno = 0;
	do {
		pid2 = waitpid(pid, status, __WALL);
	} while (pid2 == -1 && errno == EINTR);

	if (pid2 == -1 && errno != ECHILD)
		error_exit("waitpid");

	return pid2;
}

/* Wait for state changes in traced processes. Priority is given for processes
 * with pending breakpoint enabling/reinsertion. Any pending state changes are
 * "flushed" here.
 */
int ft_wait(int *status)
{
	struct process *tmp = list_of_processes;

	while (tmp) {
		if (tmp->pending_breakpoint) {
			debug(2, "Selecting priority PID %d (pending breakpoint)",
			      tmp->pid);
			return ft_waitpid(tmp->pid, status, __WALL);
		}
		tmp = tmp->next;
	}

	tmp = list_of_processes;
	while (tmp) {
		if (tmp->pending) {
			*status = tmp->pending_status;
			debug(2, "Selecting priority PID %d (pending status 0x%x)\n",
			      tmp->pid, *status);
			tmp->pending = 0;
			return tmp->pid;
		}
		tmp = tmp->next;
	}

	/* no pending breakpoints or status changes; wait for any traced PID */
	return ft_waitpid(-1, status, __WALL);
}

pid_t get_tgid(pid_t pid)
{
	char path[255];
	char *file_buf, *pos;
	FILE *fp;
	pid_t retval;

	file_buf = malloc(BUF_SIZE);
	if (file_buf == NULL)
		return -1;

	snprintf(path, sizeof(path), "/proc/%d/status", pid);
	fp = fopen(path, "r");
	if (fp == NULL) {
		perror("open");
		retval = -1;
		goto open_error;
	}

	retval = fread(file_buf, BUF_SIZE, 1, fp);
	if (retval != 1 && ferror(fp)) {
		perror("read");
		retval = -1;
		goto read_error;
	}
	file_buf[BUF_SIZE - 1] = '\0';

	retval = -1;
	pos = file_buf;
	while (1) {
		if (pos + FIELD_SIZE + 1 > file_buf + BUF_SIZE)
			break;
		if (strncmp(pos, FIELD_NAME, FIELD_SIZE) == 0) {
			char *val = strchr(pos, ':');
			assert(val != NULL);
			retval = atoi(val + 2);
			break;
		}
		pos = strchr(pos, '\n');
		if (pos == NULL)
			break;
		pos += 1;
	}

read_error:
	fclose(fp);
open_error:
	free(file_buf);

	return retval;
}

/* Get process' current state from /proc/PID/stat.
 */
static char get_process_state(struct process *proc)
{
	char procfile[64];
	FILE *filp;
	struct proc_state {
		int pid;
		char name[64];
		char state;
	} p;

	snprintf(procfile, 64, "/proc/%d/stat", proc->pid);
	filp = fopen(procfile, "r");
	if (!filp) {
		if (errno == ENOENT)
			return ' ';
		perror("get_process_state(): fopen");
		exit(1);
	}

	fscanf(filp, "%d (%64[^)]) %c", &p.pid, p.name, &p.state);
	p.name[63] = '\0';
	fclose(filp);

	return p.state;
}

/* Check whether process is inside a sleeping syscall.
 */
static int sleeping_process(struct process *proc)
{
	char ret;

	do {
		ret = get_process_state(proc);
	} while (ret == 'R');

	return (ret == 'S' || ret == 'D');
}

/* "Stop" other processes by waiting for the next state change or exit. The
 * status for the new state is saved for future usage by ft_wait().
 */
void stop_other_processes(struct process *current_proc)
{
	struct process *tmp = list_of_processes;

	while (tmp) {
		if (current_proc->pid != tmp->pid && !tmp->pending) {
			pid_t pid;

			if (sleeping_process(tmp)) {
				/* processes in sleeping state are ignored
				 * because they might wait for another (currently
				 * stopped) process to change state. This would
				 * cause the waitpid() call below to never return.
				 * The syscall that made the process sleep will
				 * eventually return, and will be stopped by
				 * ptrace anyway. */
				debug(1, "process %d is sleeping, ignoring", tmp->pid);
				tmp = tmp->next;
				continue;
			}
			pid = ft_waitpid(tmp->pid, &tmp->pending_status, __WALL);
			tmp->pending = 1;
		}
		tmp = tmp->next;
	}
}

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

char *name_from_pid(pid_t pid)
{
	char proc_exe[1024];

	if (kill(pid, 0) == 0) {
		snprintf(proc_exe, 1024, "/proc/%d/exe", pid);
		if (access(proc_exe, F_OK) < 0)
			error_exit("access");
		return realpath(proc_exe, NULL);
	}
	return NULL;
}

struct process *add_process(pid_t pid)
{
	struct process *tmp;
	pid_t tgid;

	tmp = calloc(1, sizeof(struct process));
	if (!tmp)
		error_exit("malloc");
	tmp->pid = pid;
	tmp->filename = name_from_pid(pid);
	/* Start with tracing enabled or not, depending on command
	 * line option. */
	if (arguments.enabled)
		tmp->trace_control = 1;
	tmp->next = list_of_processes;
	list_of_processes = tmp;

	tgid = get_tgid(pid);
	if (tgid != pid) {
		struct process *parent;
		parent = process_from_pid(tgid);
		if (parent != NULL) {
			tmp->child = 1;
			tmp->breakpoints = parent->breakpoints;
		}
	}
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
