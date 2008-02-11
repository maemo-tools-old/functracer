/*
 * This file is part of Functracker.
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

#include <libiberty.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "backtrace.h"
#include "debug.h"
#include "report.h"
#include "options.h"

static void rp_copy_file(const char *src, const char *dst)
{
	char line[256];
	FILE *fpi, *fpo;

	debug(3, "rp_copy_file(src=\"%s\", dst=\"%s\"", src, dst);

	fpi = fopen(src, "r");
	if (!fpi) {
		debug(1, "rp_copyfile(): fopen: \"%s\": %s", src, strerror(errno));
		return;
	}
	fpo = fopen(dst, "w");
	if (!fpo) {
		debug(1, "rp_copyfile(): fopen: \"%s\": %s", dst, strerror(errno));
		fclose(fpi);
		return;
	}
	while (fgets(line, sizeof(line), fpi)) {
		fputs(line, fpo);
	}
	fclose(fpo);
	fclose(fpi);
}

static void rp_copy_maps(struct rp_data *rd)
{
	char src[256], dst[256];

	snprintf(src, sizeof(src), "/proc/%d/maps", rd->pid);
	snprintf(dst, sizeof(dst), "%s/allocs-%d.%d.map", getenv("HOME"), rd->pid, rd->step);
	rp_copy_file(src, dst);
}

void rp_dump(struct rp_data *rd)
{
	struct rp_allocinfo *rai;
	int i = 0, j;
	char path[256];
	
	snprintf(path, sizeof(path), "%s/allocs-%d.%d.trace", getenv("HOME"), rd->pid, rd->step);
	rd->fp = fopen(path, "w");
	if (rd->fp == NULL)
		error_exit("rp_dump(): fopen");

	fprintf(rd->fp, "information about %ld allocations\n", rd->nallocs);
	rai = rd->allocs;
	while (rai) {
		fprintf(rd->fp, "%d. block at 0x%x with size %d\n", i++, rai->addr, rai->size);
		for (j = 0; j < rai->bt_depth; j++) {
			fprintf(rd->fp, "   %s\n", rai->backtrace[j]);
		}
		rai = rai->next;
	}
	fclose(rd->fp);
	rp_copy_maps(rd);
	rd->step++;
}

void rp_new_alloc(struct rp_data *rd, addr_t addr, size_t size)
{
	struct rp_allocinfo *rai;
	static int alloc_overflow = 0;

	debug(3, "rp_new_alloc(pid=%d, addr=0x%x, size=%d)", rd->pid, addr, size);
	if (rd->nallocs >= arguments.nalloc) {
		if (!alloc_overflow) {
			debug(1, "maximum number of allocations (%d) reached, new allocations will be ignored!", arguments.nalloc);
			alloc_overflow = 1;
		}
		return;
	}
	rai = xcalloc(1, sizeof(struct rp_allocinfo));
	rai->addr = addr;
	rai->size = size;
	rai->bt_depth = bt_backtrace(rd->btd, rai->backtrace, arguments.depth);
	rai->next = rd->allocs;
	rd->allocs = rai;
	rd->nallocs++;
}

void rp_init(struct process *proc)
{
	struct rp_data *rd = proc->rp_data;

	debug(3, "pid=%d", proc->pid);

	if (rd == NULL)
		rd = xcalloc(1, sizeof(struct rp_data));
	rd->pid = proc->pid;
	rd->btd = bt_init(proc->pid);
	proc->rp_data = rd;
}

void rp_reset_data(struct rp_data *rd)
{
	struct rp_allocinfo *rai, *tmp;
	int j;

	rai = rd->allocs;
	while (rai != NULL) {
		for (j = 0; j < rai->bt_depth; j++)
			free(rai->backtrace[j]);
		tmp = rai;
		rai = rai->next;
		free(tmp);
	}
	rd->allocs = NULL;
	rd->nallocs = 0;
}

void rp_finish(struct rp_data *rd)
{
	bt_finish(rd->btd);
	rp_reset_data(rd);
}
