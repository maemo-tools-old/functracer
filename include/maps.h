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

#ifndef FTK_MAPS_H
#define FTK_MAPS_H

#include <stdio.h>
#include <sys/types.h>

struct maps_data {
	FILE *fp;
	unsigned long lo, hi, off;
	char perm[4];
	unsigned int maj, min;
	int inum;
	char *path;
};

#define MAP_EXEC(md)	((md)->perm[2] == 'x')

extern int maps_init(struct maps_data *md, pid_t pid);
extern int maps_next(struct maps_data *md);
extern int maps_finish(struct maps_data *md);

#endif /* !FTK_MAPS_H */
