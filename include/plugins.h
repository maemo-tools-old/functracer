/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
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

#ifndef FTK_PLUGINS_H
#define FTK_PLUGINS_H

#include <stdbool.h>

#include "process.h"

/**
 * Structure defining symbols monitored by a plugin.
 */
struct plg_symbol {
	/* the symbol name */
	char *name;
	/* the number of hits encountered during library symbol matching.
	 * Low word contains normal hits while high word contains versioned hits. */
	int hit;
};

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
	int (*get_symbols)(struct plg_symbol **symbols);
	void (*report_init)(struct process *proc);
};


void plg_init(void);
void plg_finish(void);
void plg_function_exit(struct process *proc, const char *name);
int plg_match(const char *symname);

/**
 * Checks if all of the plugin symbols have been found in the
 * loaded libraries.
 *
 * This function will display warning message about every
 * unmatched symbol unless if silent is set to true. Consequent
 * calls won't display messages about already warned symbols.
 *
 * @param[in] silent   set 1 to suppress warning messages.
 * @return             the number of unmatched symbols.
 */
int plg_check_symbols(bool silent);

/**
 * Initializes plugin report printing.
 *
 * This function is called every time after a new report is created
 * and header record is written.
 * @param proc
 */
void plg_rp_init(struct process *proc);

#endif /* !FTK_PLUGINS_H */
