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

#ifndef FTK_DEBUG_H
#define FTK_DEBUG_H

extern void debug_(int level, const char *file, int line, const char *func,
	    const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));

extern void msg_warn(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void msg_err(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void error_file(const char *filename, const char *msg);

/* XXX deprecated */
#define error_exit(...)		msg_err(__VA_ARGS__)

#ifdef DEBUG
#define debug(level, ...)	debug_(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define debug(level, ...)	do {} while (0)
#endif

#if __GNUC__ >= 3
#define __unused	__attribute__ ((unused))
#else
#define __unused
#endif

#endif  /* !FTK_DEBUG_H */
