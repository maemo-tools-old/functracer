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

#ifndef FTK_PLUGINS_H
#define FTK_PLUGINS_H

#include "process.h"

struct plg_api {
	char *api_version;
	void (*function_entry)(struct process *proc, const char *name);
	void (*function_exit)(struct process *proc, const char *name);
	void (*process_create)(struct process *proc);
	void (*process_exec)(struct process *proc);
	void (*process_exit)(struct process *proc, int exit_code);
	void (*process_kill)(struct process *proc, int signo);
	void (*process_signal)(struct process *proc, int signo);
	void (*process_interrupt)(struct process *proc);
	void (*syscall_enter)(struct process *proc, int sysno);
	void (*syscall_exit)(struct process *proc, int sysno);
	int (*library_match)(const char *name);
};

void plg_init();
void plg_finish();
void plg_function_exit(struct process *proc, const char *name);
int plg_match(const char *symname);

#endif /* !FTK_PLUGINS_H */
