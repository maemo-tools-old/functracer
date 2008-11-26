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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"

#define MEM_API_VERSION "2.0"

static char mem_api_version[] = MEM_API_VERSION;

static void mem_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;

	addr_t retval = fn_return_value(proc);
	size_t arg0 = fn_argument(proc, 0);

	assert(proc->rp_data != NULL);
	if (strcmp(name, "__libc_malloc") == 0) {
		rp_alloc(proc, rd->rp_number, "malloc", arg0, retval);

	} else if (strcmp(name, "__libc_calloc") == 0) {
		size_t arg1 = fn_argument(proc, 1);
                rp_alloc(proc, rd->rp_number, "calloc", arg0 * arg1,
			 retval);

	} else if (strcmp(name, "__libc_memalign") == 0) {
		size_t arg1 = fn_argument(proc, 1);
                rp_alloc(proc, rd->rp_number, "memalign", arg0 * arg1,
			 retval);

	} else if (strcmp(name, "__libc_realloc") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		if (arg0 != 0) {
			/* realloc acting normally (returning same or different
			 * address) OR acting as free so showing the freeing */
			rp_free(proc, rd->rp_number++, "realloc", arg0);
		}
		if (arg1 == 0 && retval == 0) {
			/* realloc acting as free so return */
			return;
		}
		/* show a new resource allocation (can be same or different
		 * address) 
		 */
                rp_alloc(proc, rd->rp_number, "realloc", arg1, retval);

	} else if (strcmp(name, "__libc_free") == 0 ) {
		/* Suppress "free(NULL)" calls from trace output. 
		 * They are a no-op according to ISO 
		 */
		is_free = 1;
		if (arg0 == 0)
			return;
                rp_free(proc, rd->rp_number, "free", arg0);

	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	if (!is_free || arguments.enable_free_bkt)
		rp_write_backtraces(proc);
}

static int mem_library_match(const char *symname)
{
	return(strcmp(symname, "__libc_calloc") == 0 ||
	       strcmp(symname, "__libc_malloc") == 0 ||
	       strcmp(symname, "__libc_realloc") == 0 ||
	       strcmp(symname, "__libc_free") == 0 ||
	       strcmp(symname, "__libc_memalign") == 0 ||
	       strcmp(symname, "__libc_valloc") == 0 ||
	       strcmp(symname, "__libc_pvalloc") == 0 ||
	       strcmp(symname, "posix_memalign") == 0);
}

struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = mem_api_version,
		.function_exit = mem_function_exit,
		.library_match = mem_library_match,
	};
	return &ma;
}
