/*
 * This file is part of Functracker.
 *
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 1997-2007 Juan Cespedes <cespedes@debian.org>
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
 * Based on debug code from ltrace.
 */

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "debug.h"
#include "options.h"

void debug_(int level, const char *file, int line, const char *func, const char *fmt, ...)
{
	char buf[1024];
	va_list args;

	if (arguments.debug < level)
		return;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	fprintf(stderr, "DEBUG: %s:%d: %s(): %s\n", file, line, func, buf);
}

void msg_warn(const char *fmt, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	if (errno)
		perror(buf);
	else
		fprintf(stderr, "Warning: %s\n", buf);
}

void msg_err(const char *fmt, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	if (errno)
		perror(buf);
	else
		fprintf(stderr, "Error: %s\n", buf);

	/* Gracefully dettach from traced processes. */
	raise(SIGINT);
}

void error_file(const char *filename, const char *msg)
{
	msg_err("\"%s\": %s", filename, msg);
}
