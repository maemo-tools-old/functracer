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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Based on code from libleaks.
 */

#ifndef FTK_REPORT_H
#define FTK_REPORT_H

#include <stdio.h>
#include <sys/types.h>

#include "process.h"
#include "target_mem.h"

struct rp_data {
	pid_t pid;
	int rp_number; 
	int step;
	FILE *fp;
        int refcnt;
};

struct rp_alloc {
	addr_t addr;
	size_t size;
};

extern int rp_init(struct process *proc);
extern void rp_alloc(struct process *proc, int rp_number, const char *name, size_t arg0, size_t arg1);
extern void rp_alloc_details(struct process *proc, int rp_number, const char *name, size_t arg0, size_t arg1, char *details);
extern void rp_free(struct process *proc, int rp_number, const char *name, size_t arg);
extern void rp_event(struct process *proc, const char *fmt, ...);
extern void rp_write_backtraces(struct process *proc);
extern void rp_finish(struct process *proc);

#endif /* !FTK_REPORT_H */
