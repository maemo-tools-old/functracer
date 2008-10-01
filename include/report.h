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

#include "config.h"
#include "process.h"
#include "target_mem.h"

struct bt_data;

struct rp_data {
	pid_t pid;
	int rp_number; 
	int step;
	FILE *fp;
	struct bt_data *btd;
};

/* function type enumerator */
enum {
	FN_FREE,
	FN_MALLOC,
	FN_MEMALIGN,
	FN_CALLOC,
	FN_REALLOC,
};

struct rp_alloc {
	int type;
	addr_t addr;
	size_t nmemb;
	size_t boundary;
	size_t size;
	addr_t addr_new;
};

extern int rp_init(struct process *proc);
extern void rp_alloc(struct process *proc, struct rp_alloc *ra);
extern void rp_finish(struct process *proc);

#endif /* !FTK_REPORT_H */
