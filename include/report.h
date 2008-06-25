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

extern void rp_init(struct process *proc);
extern void rp_malloc(struct rp_data *rd, addr_t addr, size_t size);
extern void rp_calloc(struct rp_data *rd, addr_t addr, size_t nmemb, size_t size);
extern void rp_realloc(struct rp_data *rd, addr_t addr, addr_t addr_new, size_t size);
extern void rp_free(struct rp_data *rd, addr_t addr);
extern void rp_finish(struct rp_data *rd);

#endif /* !FTK_REPORT_H */
