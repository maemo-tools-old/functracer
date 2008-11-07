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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ptrace.h>

#include "debug.h"
#include "util.h"

long xptrace(int request, pid_t pid, void *addr, void *data)
{
	long ret;

	errno = 0;
	do {
		ret = ptrace((enum __ptrace_request)request, pid, addr, data);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1 && errno) {
		if (errno == EIO)
			msg_warn("Cannot access memory at address %p", addr);
		else
			msg_warn("ptrace");
		errno = 0;
	}

	return ret;
}
