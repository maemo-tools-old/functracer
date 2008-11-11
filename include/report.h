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
extern void rp_event(struct process *proc, const char *fmt, ...);
extern void rp_write_backtraces(struct process *proc);
extern void rp_finish(struct process *proc);

#endif /* !FTK_REPORT_H */
