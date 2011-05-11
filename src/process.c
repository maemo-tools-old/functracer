/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008-2011 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include <limits.h>
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

static struct process *list_of_processes = NULL;

static pid_t get_pid_from_status(pid_t pid, const char *field_name)
{
	char path[255];
	char *file_buf, *pos;
	FILE *fp;
	pid_t retval;
	size_t field_size = strlen(field_name);

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
		if (pos + field_size + 1 > file_buf + BUF_SIZE)
			break;
		if (strncmp(pos, field_name, field_size) == 0) {
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

pid_t get_tgid(pid_t pid)
{
	return get_pid_from_status(pid, "Tgid:");
}

pid_t get_ppid(pid_t pid)
{
	return get_pid_from_status(pid, "PPid:");
}

void for_each_process(for_each_process_t callback, int value)
{
	struct process *tmp;

	tmp = list_of_processes;
	while (tmp) {
		callback(tmp, value);
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
	char proc_exe[PATH_MAX];
	char *buf;
	ssize_t len;

	if (kill(pid, 0) == 0) {
		snprintf(proc_exe, sizeof(proc_exe), "/proc/%d/exe", pid);
		if (access(proc_exe, F_OK) < 0)
			error_exit("access");
		if ((buf = malloc(PATH_MAX)) == NULL)
			error_exit("malloc");
		if ((len = readlink(proc_exe, buf, PATH_MAX)) <= 0)
			error_exit("readlink");
		buf[len] = '\0';
		/* caller frees buf */
		return buf;
	}
	return NULL;
}

char *cmd_from_pid(pid_t pid, int noargs)
{
	char proc_cmd[1024], buf[1024];
	char *pos, *new_pos;
	FILE *fp;
	int ret;

	if (kill(pid, 0) == 0) {
		snprintf(proc_cmd, sizeof(proc_cmd), "/proc/%d/cmdline", pid);
		if (access(proc_cmd, F_OK) < 0)
			error_exit("access");
		fp = fopen(proc_cmd, "r");
		if (fp == NULL)
			error_exit("fopen");
		memset(buf, 0, sizeof(buf));
		ret = fscanf(fp, "%1024c", buf);
		if (ret < 0 || ferror(fp)) {
			return strdup("<none>");
		}
		fclose(fp);
		/* Truncate string */
		buf[sizeof(buf) - 2] = '\0';
		buf[sizeof(buf) - 1] = '\0';
		pos = buf;
		while (!noargs) {
			new_pos = rawmemchr(pos, '\0');
			if (new_pos == pos) {
				/* Remove last whitespace */
				*(new_pos - 1) = '\0';
				break;
			}
			*new_pos = ' ';
			pos = new_pos + 1;

		}
		return strdup(buf);
	}
	return strdup("<none>");
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
		if (tmp->parent)
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
