/*
 * memory is functracer module used to track memory allocation
 * and release.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2008,2010 by Nokia Corporation
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

#define MEM_API_VERSION "2.0"

static char mem_api_version[] = MEM_API_VERSION;

static sp_rtrace_resource_t res_memory = {
		.id = 1,
		.type = "memory",
		.desc = "memory allocation in bytes",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};

static void mem_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;

	addr_t retval = fn_return_value(proc);
	size_t arg0 = fn_argument(proc, 0);

	assert(proc->rp_data != NULL);
	if (strcmp(name, "__libc_malloc") == 0) {
		/* suppress allocation failures */
		if (retval == 0) return;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "malloc",
				.res_size = arg0,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "__libc_calloc") == 0) {
		size_t arg1;
		/* suppress allocation failures */
		if (retval == 0) return;
		arg1 = fn_argument(proc, 1);

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "calloc",
				.res_size = arg0 * arg1,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "__libc_memalign") == 0) {
		size_t arg1;
		/* suppress allocation failures */
		if (retval == 0) return;
		arg1 = fn_argument(proc, 1);

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "memalign",
				.res_size = arg1,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "__libc_realloc") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		if (arg0 != 0) {
			/* realloc acting normally (returning same or different
			 * address) OR acting as free so showing the freeing */
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_FREE,
					.index = rd->rp_number++,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "realloc",
					.res_size = 0,
					.res_id = (pointer_t)arg0,
			};
			sp_rtrace_print_call(rd->fp, &call);

			if (arguments.enable_free_bkt) rp_write_backtraces(proc);
			else sp_rtrace_print_comment(rd->fp, "\n"); 
			
		}
		if (arg1 == 0 && retval == 0) {
			/* realloc acting as free so return */
			return;
		}
		/* show a new resource allocation (can be same or different
		 * address)
		 */
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "realloc",
				.res_size = arg1,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "__libc_free") == 0 ) {
		/* Suppress "free(NULL)" calls from trace output.
		 * They are a no-op according to ISO
		 */
		is_free = 1;
		if (arg0 == 0)
			return;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "free",
				.res_size = 0,
				.res_id = (pointer_t)arg0,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "posix_memalign") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		size_t arg2 = fn_argument(proc, 2);
		if (retval != 0)
			return;
		/* posix_memalign() stores the allocated memory pointer on the
		 * first argument (of type (void **)) */
		retval = trace_mem_readw(proc, arg0);
		/* ignore allocation failures */
		if (retval == 0) return;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "posix_memalign",
				.res_size = arg2,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "__libc_valloc") == 0 || strcmp(name, "valloc") == 0) {
		/* suppress allocation failures */
		if (retval == 0) return;
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "valloc",
				.res_size = arg0,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	}
	else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	if (!is_free || arguments.enable_free_bkt) {
		rp_write_backtraces(proc);
	}
	else {
		sp_rtrace_print_comment(rd->fp, "\n"); 
	}
	
}

static int mem_library_match(const char *symname)
{
	return(strcmp(symname, "__libc_calloc") == 0 ||
	       strcmp(symname, "__libc_malloc") == 0 ||
	       strcmp(symname, "__libc_realloc") == 0 ||
	       strcmp(symname, "__libc_free") == 0 ||
	       strcmp(symname, "__libc_memalign") == 0 ||
				 strcmp(symname, "posix_memalign") == 0 ||
				 strcmp(symname, "valloc") == 0);
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
		.library_match = mem_library_match,
		.report_init = mem_report_init,
	};
	return &ma;
}
