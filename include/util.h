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

#ifndef FTK_UTIL_H
#define FTK_UTIL_H

extern long xptrace(int request, pid_t pid, void *addr, void *data);

#ifndef ARRAY_SIZE
 #define ARRAY_SIZE(x)   sizeof(x) / sizeof(x[0])
#endif

/* C++ name demangking support */
#define DMGL_PARAMS  (1 << 0) /* Include function args */
#define DMGL_ANSI  (1 << 1)   /* Include const, volatile, etc */

extern char* cplus_demangle(const char* symbol, int flags);


/**
 * Compare text string against a pattern.
 *
 * The pattern has following format: <text>[*]
 * Where ending '*' matches all strings starting with <text>
 * @param pattern  - the pattern to compare against.
 * @param text     - the text string to compare.
 * @return         <0 - pattern is 'less' than the text.
 *                  0 - text matches the pattern.
 *                 >0 - pattern is 'greater' than the text.
 */
int strcmp_pattern(const char* pattern, const char* text);


#endif /* !FTK_UTIL_H */
