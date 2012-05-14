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

#ifndef FT_ARCH_DEFS_I386_H
#define FT_ARCH_DEFS_I386_H

#define MAX_BT_DEPTH		256	/* maximum backtrace depth */
#define DECR_PC_AFTER_BREAK	1	/* decrement after breakpoint */
#define MAX_INSN_SIZE		32	/* maximum instruction size */
#define FT_PTRACE_SINGLESTEP	PTRACE_SINGLESTEP

#endif /* !FT_ARCH_DEFS_I386_H */
