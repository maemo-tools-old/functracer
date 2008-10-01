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

#ifndef FTK_FUNCTION_H
#define FTK_FUNCTION_H

#include "target_mem.h"

struct process;

extern long fn_argument(struct process *proc, int arg_num);
extern long fn_return_value(struct process *proc);
extern void fn_get_return_address(struct process *proc, addr_t *addr);
extern void fn_set_return_address(struct process *proc, addr_t addr);
extern void fn_do_return(struct process *proc);
extern int fn_callstack_push(struct process *proc, char *fn_name);
extern void fn_callstack_pop(struct process *proc);
extern void fn_callstack_restore(struct process *proc, int original);
extern char *fn_name(struct process *proc);

#endif /* !FTK_FUNCTION_H */
