/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008,2010 by Nokia Corporation
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
 */

#include <assert.h>
#include <libiberty.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>

#include "arch-defs.h"
#include "backtrace.h"
#include "config.h"
#include "debug.h"
#include "report.h"
#include "options.h"
#include "plugins.h"

#define FNAME_FMT "%s/%d-%d.rtrace.txt"

void rp_write_backtraces(struct process *proc)
{
	int bt_depth, i;
	char *names[MAX_BT_DEPTH];
	void *frames[MAX_BT_DEPTH];
	struct rp_data *rd = proc->rp_data;

	bt_depth = bt_backtrace(proc->bt_data, frames, names, arguments.depth);

	debug(3, "rp_write_backtraces(pid=%d)", rd->pid);

	sp_rtrace_ftrace_t trace = {
			.nframes = bt_depth,
			.frames = (pointer_t*)frames,
			.resolved_names = arguments.resolve_name ? names : NULL,
	};

	sp_rtrace_print_trace(rd->fp, &trace);

	if (arguments.resolve_name) {
		for (i = 0; i < bt_depth; i++) {
			free(names[i]);
		}
	}
}


static int rp_write_header(struct process *proc)
{
	struct rp_data *rd = proc->rp_data;
	char path[256], *buf, pid_s[8], btdepth_s[8];

	if (arguments.save_to_file) {
		struct stat buf;
		/* Do not overwrite existing trace file so it will use a
		 * non-existing path.
		 */
		do {
			snprintf(path, sizeof(path), FNAME_FMT,
				 arguments.path ? : getenv("HOME"), rd->pid,
				 rd->step++);
		} while (stat(path, &buf) == 0);
		rd->step--;

		rd->fp = fopen(path, "a");
		if (rd->fp == NULL) {
			error_exit("rp_init(): fopen");
			bt_finish(proc->bt_data);
			free(rd);
			proc->rp_data = rd = NULL;
			return -1;
		}
	} else
		rd->fp = stdout;

	buf = cmd_from_pid(proc->pid, 1);
	sprintf(pid_s, "%d", proc->pid);
	sprintf(btdepth_s, "%d", arguments.depth);

	sp_rtrace_header_t header =  {
			.fields = {
					PACKAGE_STRING,  // HEADER_VERSION
					BUILD_ARCH,      // HEADER_ARCH
					NULL,            // HEADER_TIMESTAMP
					buf,             // HEADER_PROCESS
					pid_s,           // HEADER_PID
					NULL,            // HEADER_FILTER
					btdepth_s,       // HEADER_BACKTRACE_DEPTH
					PACKAGE_STRING,  // HEADER_ORIGIN
			},
	};

	sp_rtrace_print_header(rd->fp, &header);
	free(buf);

	return 0;
}

int rp_init(struct process *proc)
{
	struct rp_data *rd;

	debug(3, "pid=%d", proc->pid);

	if (proc->parent != NULL)
		rd = proc->parent->rp_data;
	else
		rd = proc->rp_data;
	if (rd == NULL) {
		rd = xcalloc(1, sizeof(struct rp_data));
		if (proc->parent != NULL)
			rd->pid = proc->parent->pid;
		else
			rd->pid = proc->pid;
	}
	if (proc->parent != NULL)
		proc->parent->rp_data = rd;
	proc->rp_data = rd;
	if (rd->refcnt++ == 0) {
		int ret = rp_write_header(proc);
		if (ret < 0)
			return ret;
		plg_rp_init(proc);
	}
	proc->bt_data = bt_init(proc->pid);
	if (arguments.verbose)
		fprintf(stderr, "Started tracing %d\n", proc->pid);
	return 0;
}

void rp_finish(struct process *proc)
{
	struct rp_data *rd = proc->rp_data;

	assert(rd != NULL);
	bt_finish(proc->bt_data);
	assert(rd->refcnt > 0);
	if (--rd->refcnt == 0) {
		rd->step++;
		if (arguments.save_to_file)
			fclose(rd->fp);
	}
	if (arguments.verbose) {
		char fname[256];
		if (arguments.save_to_file)
			snprintf(fname, sizeof(fname), FNAME_FMT,
				 arguments.path ? : getenv("HOME"), rd->pid,
				 rd->step - 1);
		else
			snprintf(fname, sizeof(fname), "stdout");
		fprintf(stderr, "Stopped tracing %d, trace saved to "
			"%s\n", proc->pid, fname);
	}
	if (rd->refcnt == 0) {
		free(rd);
	}
}
