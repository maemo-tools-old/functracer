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

#ifndef TT_PTRACE_H
#define TT_PTRACE_H

#include <stdint.h>

#include "process.h"

/*typedef void *addr_t;*/
typedef uintptr_t addr_t;

extern long trace_mem_readw(struct process *proc, addr_t addr);
extern void trace_mem_writew(struct process *proc, addr_t addr, long w);
extern long trace_user_readw(struct process *proc, long offset);
extern void trace_user_writew(struct process *proc, long offset, long w);
extern void trace_mem_read(struct process *proc, addr_t addr, void *buf, size_t count);
extern void trace_mem_write(struct process *proc, addr_t addr, void *buf, size_t count);
extern void trace_getregs(struct process *proc, void *regs);
extern void trace_setregs(struct process *proc, void *regs);

#endif /* TT_PTRACE_H */
