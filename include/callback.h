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

#ifndef FTK_CALLBACK_H
#define FTK_CALLBACK_H

#include "process.h"
#include "target_mem.h"

struct callback {
	struct {
		void (*enter)(struct process *proc, const char *name);
		void (*exit)(struct process *proc, const char *name);
	} function;
	struct {
		void (*create)(struct process *proc);
		void (*exec)(struct process *proc);
		void (*exit)(struct process *proc, int exit_code);
		void (*fork)(struct process *proc, pid_t child_pid);
		void (*kill)(struct process *proc, int signo);
		void (*signal)(struct process *proc, int signo);
		void (*interrupt)(struct process *proc);
	} process;
	struct {
		void (*enter)(struct process *proc, int sysno);
		void (*exit)(struct process *proc, int sysno);
	} syscall;
	struct {
		void (*load)(struct process *proc, addr_t start_addr,
			     addr_t end_addr, char *path);
	} library;

};

extern void cb_init(void);
extern void cb_finish(void);
extern struct callback *cb_get(void);

#endif /* !FTK_CALLBACK_H */
