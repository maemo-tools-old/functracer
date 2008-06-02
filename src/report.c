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
	snprintf(dst, sizeof(dst), "%s/allocs-%d.%d.map",
			arguments.path ? : getenv("HOME"), rd->pid, rd->step);
	rp_copy_file(src, dst);
}

void rp_alloc(struct rp_data *rd, const char *name, addr_t addr, size_t size)
{
	int bt_depth, j;
	char *backtrace[MAX_BT_DEPTH];

	debug(3, "rp_alloc(pid=%d, name=%s, addr=0x%x, size=%d)", rd->pid, name, addr, size);

	bt_depth = bt_backtrace(rd->btd, backtrace, arguments.depth);

	fprintf(rd->fp, "%d. %s: block at 0x%x with size %d\n", rd->rp_number++, name, addr, size);

	for (j = 0; j < bt_depth; j++) {
		fprintf(rd->fp, "   %s\n", backtrace[j]);
		free(backtrace[j]);
	}
}

void rp_free(struct rp_data *rd, addr_t addr)
{
	int bt_depth, j;
	char *backtrace[MAX_BT_DEPTH];

	debug(3, "rp_free(pid=%d, addr=0x%x)", rd->pid, addr);

	bt_depth = bt_backtrace(rd->btd, backtrace, arguments.depth);

	fprintf(rd->fp, "%d. free: block at 0x%x\n", rd->rp_number++, addr);

	for (j = 0; j < bt_depth; j++) {
		fprintf(rd->fp, "   %s\n", backtrace[j]);
		free(backtrace[j]);
	}
}

void rp_init(struct process *proc)
{
	struct rp_data *rd = proc->rp_data;
	char path[256];

	debug(3, "pid=%d", proc->pid);

	if (rd == NULL)
		rd = xcalloc(1, sizeof(struct rp_data));
	rd->pid = proc->pid;
	rd->btd = bt_init(proc->pid);
	rd->rp_number = 0;
	proc->rp_data = rd;

	if (arguments.save_to_file) {
		snprintf(path, sizeof(path), "%s/allocs-%d.%d.trace",
				arguments.path ? : getenv("HOME"), rd->pid,
				rd->step);

		rd->fp = fopen(path, "a");
		if (rd->fp == NULL)
			error_exit("rp_init(): fopen");
	} else
		rd->fp = stdout;
}

void rp_finish(struct rp_data *rd)
{
	rp_copy_maps(rd);
	rd->step++;
	bt_finish(rd->btd);

	if (arguments.save_to_file)
		fclose(rd->fp);
}
