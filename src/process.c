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

#include "breakpoint.h"
#include "debug.h"
#include "options.h"
#include "process.h"
#include "report.h"
#include "trace.h"

#define BUF_SIZE	4096
#define FIELD_NAME	"Tgid:"
#define FIELD_SIZE	(sizeof(FIELD_NAME) - 1)

static struct process *list_of_processes = NULL;

static pid_t get_tgid(pid_t pid)
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

void for_each_process(for_each_process_t callback, void *arg)
{
	struct process *tmp;

	tmp = list_of_processes;
	while (tmp) {
		callback(tmp, arg);
		tmp = tmp->next;
	}
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
		tmp->parent = process_from_pid(tgid);
		/* Set tracing status according to whether parent has tracing
		 * enabled or not. */
		tmp->trace_control = tmp->parent->trace_control;
	}

	debug(1, "Adding PID %d, filename = \"%s\"", pid, tmp->filename);

	return tmp;
}

static void free_process(struct process *proc, struct process **prev_next)
{
	*prev_next = proc->next;
	if (proc->filename)
		free(proc->filename);
	free(proc);
}

void remove_process(struct process *proc)
{
	struct process *tmp;

	debug(1, "Removing PID %d", proc->pid);

	if (list_of_processes->pid == proc->pid) {
		free_process(list_of_processes, &list_of_processes);
		return;
	}
	tmp = list_of_processes;
	while (tmp->next) {
		if (tmp->next->pid == proc->pid) {
			free_process(tmp->next, &tmp->next);
			return;
		}
		tmp = tmp->next;
	}
}

void remove_all_processes(void)
{
#if 0
	struct process *tmp = list_of_processes;

	while (tmp) {
		fprintf(stderr, "removing dangling process: %d\n", tmp->pid);
		rp_finish(tmp);
		free_process(tmp, &tmp);
	}
#endif
}
