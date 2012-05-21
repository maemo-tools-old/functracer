/*
 * memory is functracer module used to track memory allocation
 * and release.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2008-2012 by Nokia Corporation
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "target_mem.h"
#include "context.h"
#include "util.h"

#define MEM_API_VERSION "2.0"

static char mem_api_version[] = MEM_API_VERSION;

static sp_rtrace_resource_t res_memory = {
		.id = 1,
		.type = "memory",
		.desc = "memory allocation in bytes",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};

static void write_function(struct process *proc, const char *name, unsigned int type, size_t size, pointer_t id)
{
	struct rp_data *rd = proc->rp_data;
	sp_rtrace_fcall_t call = {
		.type = type,
		.index = rd->rp_number,
		.context = context_mask,
		.timestamp = RP_TIMESTAMP,
		.name = (char *)name,	/* not modified by libsp-rtrace */
		.res_size = size,
		.res_id = id
	};
	sp_rtrace_print_call(rd->fp, &call);
	rp_write_backtraces(proc, &call);
	(rd->rp_number)++;
}

static void mem_function_exit(struct process *proc, const char *name)
{
	addr_t retval = fn_return_value(proc);
	size_t arg0 = fn_argument(proc, 0);

	assert(proc->rp_data != NULL);
	if (strcmp(name, "__libc_malloc") == 0) {
		/* suppress allocation failures */
		if (retval) {
			write_function(proc, "malloc", SP_RTRACE_FTYPE_ALLOC, arg0, retval);
		}
	} else if (strcmp(name, "__libc_free") == 0 ) {
		/* Suppress "free(NULL)" calls from trace output.
		 * They are a no-op according to ISO
		 */
		if (arg0) {
			write_function(proc, "free", SP_RTRACE_FTYPE_FREE, 0, arg0);
		}
	} else if (strcmp(name, "__libc_calloc") == 0) {
		/* suppress allocation failures */
		if (retval) {
			size_t arg1 = fn_argument(proc, 1);
			write_function(proc, "calloc", SP_RTRACE_FTYPE_ALLOC, arg0*arg1, retval);
		}
	} else if (strcmp(name, "__libc_realloc") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		if (arg0 != 0) {
			/* realloc acting normally (returning same or different
			 * address) OR acting as free so showing the freeing
			 */
			write_function(proc, "realloc", SP_RTRACE_FTYPE_FREE, 0, arg0);
		}
		if (arg1 == 0 && retval == 0) {
			/* realloc acting as free so return */
			return;
		}
		/* show a new resource allocation
		 * (can be same or different address)
		 */
		write_function(proc, "realloc", SP_RTRACE_FTYPE_ALLOC, arg1, retval);

	} else if (strcmp(name, "posix_memalign") == 0) {
		if (retval != 0) {
			return;
		}
		/* posix_memalign() stores the allocated memory pointer on
		 * the first argument (of type (void **))
		 */
		retval = trace_mem_readw(proc, arg0);
		/* ignore allocation failures */
		if (retval) {
			size_t arg2 = fn_argument(proc, 2);
			write_function(proc, "posix_memalign", SP_RTRACE_FTYPE_ALLOC, arg2, retval);
		}
	} else if (strcmp(name, "__libc_memalign") == 0) {
		/* suppress allocation failures */
		if (retval) {
			size_t arg1 = fn_argument(proc, 1);
			write_function(proc, "memalign", SP_RTRACE_FTYPE_ALLOC, arg1, retval);
		}
	} else if (strcmp(name, "__libc_valloc") == 0 || strcmp(name, "valloc") == 0) {
		/* suppress allocation failures */
		if (retval) {
			write_function(proc, "valloc", SP_RTRACE_FTYPE_ALLOC, arg0, retval);
		}
	} else {
		msg_warn("unexpected function exit (%s)\n", name);
	}
}

static struct plg_symbol symbols[] = {
		{.name = "__libc_malloc", .hit = 0},
		{.name = "__libc_free", .hit = 0},
		{.name = "__libc_calloc", .hit = 0},
		{.name = "__libc_realloc", .hit = 0},
		{.name = "posix_memalign", .hit = 0},
		{.name = "__libc_memalign", .hit = 0},
		{.name = "valloc", .hit = 0},
};

static int get_symbols(struct plg_symbol **syms)
{
	*syms = symbols;
	return ARRAY_SIZE(symbols);
}

static void mem_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_memory);
}

struct plg_api *init(void)
{
	static struct plg_api ma = {
		.api_version = mem_api_version,
		.function_exit = mem_function_exit,
		.get_symbols = get_symbols,
		.report_init = mem_report_init,
	};
	return &ma;
}
