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

#ifndef TT_PROCESS_H
#define TT_PROCESS_H

#include <sys/types.h>

struct dict;
struct bt_data;
struct rp_data;
struct solib_list;

struct callstack {
	void *data[3];
	struct callstack *next;
};

struct process;

/**
 * Shared data between process and its threads.
 *
 * The shared data is allocated when parent process is created and
 * freed when no more processes refers to it.
 */
struct process_shared {
	struct dict *breakpoints;
	struct solib_list *solib_list;
	struct ssol *ssol;
	int ref_count;
	struct process* main;
};

struct process {
	pid_t pid;
	char *filename;
	struct process_shared* shared;
	struct bt_data *bt_data;
	struct rp_data *rp_data;
	struct callstack *callstack;
	int callstack_depth;
	int trace_control;
	int singlestep;
	int exiting;
	int stopping;
	int in_syscall;
	struct process *parent;
	struct process *next;
};

typedef void (*for_each_process_t)(struct process *, int value);

extern void for_each_process(for_each_process_t callback, int value);
extern struct process *process_from_pid(pid_t pid);
extern char *name_from_pid(pid_t pid);
extern char *cmd_from_pid(pid_t pid, int nargs);
extern struct process *add_process(pid_t pid);
extern void remove_process(struct process *proc);
extern void remove_all_processes(void);
extern pid_t get_tgid(pid_t pid);
extern pid_t get_ppid(pid_t pid);

#endif /* TT_PROCESS_H */
