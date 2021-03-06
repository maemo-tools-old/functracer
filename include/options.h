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

#ifndef TT_OPTIONS_H
#define TT_OPTIONS_H

#include <sys/types.h>
#include <stdbool.h>
#include <sp_rtrace_filter.h>

#define MAX_NPIDS 20
#define OPT_USAGE -3

struct arguments {
	char **remaining_args;
	pid_t pid[MAX_NPIDS];
	int npids;
	int depth;
	int debug;
	int enabled;
	int save_to_file;
	int resolve_name;
	int enable_free_bkt;
	int time;
	int verbose;
	const char *plugin;
	char *path;
	/* audit option arguments */
	char *audit;
	/* allocation size filter option arguments */
	char *filter_size;
	/* allocation size filter */
	sp_rtrace_filter_t *filter;
	/* don't check if monitored symbols are located */
	bool skip_symbol_check;
	/* set to true when functracer is stopping */
	bool stopping;
};

extern struct arguments arguments;

extern int process_options(int argc, char *argv[], int *remaining);

#endif /* TT_OPTIONS_H */
