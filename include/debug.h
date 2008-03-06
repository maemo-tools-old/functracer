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

#ifndef FTK_DEBUG_H
#define FTK_DEBUG_H

extern void debug_(int level, const char *file, int line, const char *func,
	    const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));
#define msg_dbg(level, ...) debug_(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

extern void msg_warn(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void msg_err(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void error_file(const char *filename, const char *msg);

/* XXX deprecated */
#define error_exit(...)		msg_err(__VA_ARGS__)
#define debug(level, ...)	msg_dbg(level, __VA_ARGS__)

#if __GNUC__ >= 3
#define __unused	__attribute__ ((unused))
#else
#define __unused
#endif

#endif  /* !FTK_DEBUG_H */
