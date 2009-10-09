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

#include "libcallchain_cpp.h"

static void do_backtrace(void)
{
        free(malloc(111));
}

// This is the "terminal" class, usually inlined
class X {
public:
        void x(int i, ...) { do_backtrace(); libA().lib_f(1, this); }
};

#define def_class(cls1, cls2, meth1, meth2) \
class cls1 { \
public: \
        void meth1(int i, ...) { cls2().meth2(i + 1, this); } \
};

def_class(C, X, h, x)
def_class(B, C, g, h)
def_class(A, B, f, g)

int main(void)
{
        A().f(1, &main);
        return 0;
}
