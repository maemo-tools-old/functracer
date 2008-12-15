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

#include <stdlib.h>

extern void lib_f(int i, ...);

static void do_backtrace(void)
{
	free(malloc(123));
}

/* Note: this function is usually inlined */
static void x(int i, ...)
{
	do_backtrace();
	lib_f(1, &x);
}

#define def_func(func, callee) \
static void func(int i, ...) { \
	callee(i + 1, &func); \
}

def_func(h, x)
def_func(g, h)
def_func(f, g)

int main(void)
{
	f(1, &main);
	return 0;
}
