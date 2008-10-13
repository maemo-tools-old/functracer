/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2008 Free Software Foundation, Inc.
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
 * Based on backtrace code from GDB.
 */

#ifndef FTK_SOLIB_H
#define FTK_SOLIB_H

#include "process.h"
#include "target_mem.h"

struct solib_list {
	addr_t start_addr;
	addr_t end_addr;
	char *path;
	struct solib_list *next;
};

typedef void (*new_sym_t)(struct process *, const char *, const char *,
			  addr_t);

extern void solib_update_list(struct process *proc, new_sym_t callback);
extern addr_t solib_dl_debug_address(struct process *proc);
extern void free_all_solibs(struct process *proc);

#endif /* !FTK_SOLIB_H */
