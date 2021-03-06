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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "debug.h"
#include "maps.h"

int maps_init(struct maps_data *md, pid_t pid)
{
	char path[PATH_MAX];
	FILE *fp;

	snprintf(path, sizeof(path), "/proc/%d/maps", pid);
	fp = fopen(path, "r");
	if (fp == NULL) {
		msg_err("fopen: %s", path);
		return -1;
	}
	md->fp = fp;
	return 0;
}

int maps_next(struct maps_data *md)
{
	static char path_buf[PATH_MAX];

	if (fscanf(md->fp, "%lx-%lx %4s %lx %x:%x %d%*[ ]", &md->lo, &md->hi,
	    md->perm, &md->off, &md->maj, &md->min, &md->inum) == EOF) {
		/* no more lines to read */
		return 0;
	}
	if (fgets(path_buf, sizeof(path_buf), md->fp) == NULL && errno != EOF) {
		msg_err("fgets");
		return -1;
	}
	if (path_buf[strlen(path_buf) - 1] != '\n') {
		msg_err("truncated path returned (too long path?)");
		return -1;
	}

	/* remove trailing newline */
	path_buf[strlen(path_buf) - 1] = '\0';

	/* return pointer to static data, avoiding unnecessary allocations */
	/* string in path_buf is only valid until next maps_next() call */
	md->path = path_buf;

	return 1;
}

int maps_finish(struct maps_data *md)
{
	fclose(md->fp);

	return 0;
}
