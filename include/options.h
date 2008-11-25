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

#define MAX_NPIDS 20
#define OPT_USAGE -3

struct arguments {
	pid_t pid[MAX_NPIDS];
	int npids;
	int depth;
	int debug;
	int enabled;
	int save_to_file;
	int resolve_name;
	int enable_free_bkt;
	int time;
	char *plugin;
	char *path;
};

extern struct arguments arguments;

extern int process_options(int argc, char *argv[], int *remaining);

#endif /* TT_OPTIONS_H */
