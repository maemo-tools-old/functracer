/*
 * memory is a functracer module
 * Memory allocation/deallocation tracing module. Tracks calls of malloc, calloc, realloc, posix_memalign and free functions.
 *
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

#define MODULE_API_VERSION "2.0"

static char module_api_version[] = MODULE_API_VERSION;

unsigned int res_memory = 1 << 0;

static void module_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;
	assert(proc->rp_data != NULL);
	addr_t rc = fn_return_value(proc);
	
	if (false);
	else if (strcmp(name, "__libc_malloc") == 0) {
		unsigned int res_size = (unsigned int)fn_argument(proc, 0);
		void* res_id = (void*)rc;
		if (rc == 0) return;
		sp_rtrace_print_call(rd->fp, rd->rp_number, context_mask, res_memory, "malloc", res_size, res_id, NULL);
	}
	else if (strcmp(name, "__libc_calloc") == 0) {
		unsigned int res_size = (unsigned int)fn_argument(proc, 0) * fn_argument(proc, 1)  ;
		void* res_id = (void*)rc;
		if (rc == 0) return;
		sp_rtrace_print_call(rd->fp, rd->rp_number, context_mask, res_memory, "calloc", res_size, res_id, NULL);
	}
	else if (strcmp(name, "posix_memalign") == 0) {
		unsigned int res_size = (unsigned int)fn_argument(proc, 2);
		void* res_id = (void*)trace_mem_readw(proc, fn_argument(proc, 0));
		if (rc != 0) return;
		sp_rtrace_print_call(rd->fp, rd->rp_number, context_mask, res_memory, "posix_memalign", res_size, res_id, NULL);
	}
	else if (strcmp(name, "__libc_free") == 0) {
		unsigned int res_size = (unsigned int)0;
		void* res_id = (void*)fn_argument(proc, 0);
		is_free = 1;
		sp_rtrace_print_call(rd->fp, rd->rp_number, context_mask, res_memory, "free", res_size, res_id, NULL);
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

static int module_library_match(const char *symname)
{
	return(
		strcmp(symname, "__libc_malloc") == 0 ||
		strcmp(symname, "__libc_calloc") == 0 ||
		strcmp(symname, "posix_memalign") == 0 ||
		strcmp(symname, "__libc_free") == 0 ||
		false);
}

static void module_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, res_memory, "memory", "memory allocation in bytes", SP_RTRACE_RESOURCE_DEFAULT);
}

struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = module_api_version,
		.function_exit = module_function_exit,
		.library_match = module_library_match,
		.report_init = module_report_init,
	};
	return &ma;
}