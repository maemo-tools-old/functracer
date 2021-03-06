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

#include <stdlib.h>

static void lib_do_backtrace(void)
{
	free(malloc(456));
}

/* Note: this function is usually inlined */
static void lib_x(int i, ...)
{
	lib_do_backtrace();
}

#define def_func(func, callee) \
void func(int i, ...) { \
	callee(i + 1, &func); \
}

def_func(lib_h, lib_x)
def_func(lib_g, lib_h)
def_func(lib_f, lib_g)

#undef def_func
