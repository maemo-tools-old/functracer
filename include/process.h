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
	int trace_control;
	int child;
	struct process *next;
};

extern struct process *get_list_of_processes(void);
extern struct process *process_from_pid(pid_t pid);
extern char *name_from_pid(pid_t pid);
extern struct process *add_process(pid_t pid);
extern void remove_process(struct process *proc);

#endif /* TT_PROCESS_H */
