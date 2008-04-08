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
extern void fn_return_address(struct process *proc, addr_t *addr);
extern void fn_save_arg_data(struct process *proc);
extern void fn_invalidate_arg_data(struct process *proc);

#endif /* !FTK_FUNCTION_H */
