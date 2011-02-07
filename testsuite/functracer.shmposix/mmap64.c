/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2011 by Nokia Corporation
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#define SEGMENT_NAME	"/shmposix"

int main(int argc, char* argv[])
{
	int fd = open(argv[0], O_RDONLY);
	if (fd == -1) return 0;
	struct stat fs;
	fstat(fd, &fs);
	void* ptr = mmap64(NULL, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (ptr != MAP_FAILED) {
		munmap(ptr, fs.st_size);
	}

	close(fd);
	return 0;
}
