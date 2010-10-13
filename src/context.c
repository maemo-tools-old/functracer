/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2010 by Nokia Corporation
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sp_rtrace_formatter.h>

#include "report.h"
#include "process.h"
#include "function.h"
#include "target_mem.h"
#include "context.h"

int context_mask = 0;

int context_function_exit(struct process *proc, const char *name)
{
	if (!strcmp(name, "sp_context_create")) {
		addr_t retval = fn_return_value(proc);

		char context_name[32] = "uninitialized";
		unsigned int context_id = fn_return_value(proc);
		trace_mem_readstr(proc, fn_argument(proc, 0), context_name, sizeof(context_name));
		sp_rtrace_print_context(proc->rp_data->fp, context_id, context_name);
		return 0;
	}
	else if (!strcmp(name, "sp_context_enter")) {
		unsigned int context_id = fn_argument(proc, 0);
		context_mask |= context_id;
		return 0;
	}
	else if (!strcmp(name, "sp_context_exit")) {
		unsigned int context_id = fn_argument(proc, 0);
		context_mask &= (~context_id);
		return 0;
	}
	return EINVAL;
}

int context_match(const char *symname)
{
	return  strcmp(symname, "sp_context_create") == 0 ||
			strcmp(symname, "sp_context_enter") == 0 ||
			strcmp(symname, "sp_context_exit") == 0;
}
