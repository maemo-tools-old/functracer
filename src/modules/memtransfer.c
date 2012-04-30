/*
 * memtransfer is functracer module used to track memory transfer operations.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2009-2012 by Nokia Corporation
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
#include <wchar.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "context.h"
#include "util.h"

#define WSIZE 4
#define POINTER 4
#define MEMTRANSFER_API_VERSION "2.0"

static char memtransfer_api_version[] = MEMTRANSFER_API_VERSION;

static sp_rtrace_resource_t res_memory = {
		.id = 1,
		.type = "memtransfer",
		.desc = "memory read/write/set/copy operations in bytes",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};


static void memtransfer_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;

	size_t arg1;
	size_t arg2;
	size_t arg3;
	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);

	if (strcmp(name, "memcpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "memcpy",
				.res_size = arg2,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "mempcpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "mempcpy",
				.res_size = arg2,
				.res_id = fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "memmove") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "memmove",
				.res_size = arg2,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "memccpy") == 0) {
		arg3 = fn_argument(proc, 3);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "memccpy",
				.res_size = arg3,
				.res_id = fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "memset") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "memset",
				.res_size = arg2,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strcpy") == 0) {
		int len = trace_mem_readstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strcpy",
				.res_size = len,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strncpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strncpy",
				.res_size = arg2,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "stpcpy") == 0) {
		int len = trace_mem_readstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "stpcpy",
				.res_size = len,
				.res_id = fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "stpncpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "stpncpy",
				.res_size = arg2,
				.res_id = fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strcat") == 0) {
		int len = trace_mem_readstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strcat",
				.res_size = len,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strncat") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strncat",
				.res_size = arg2,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "bcopy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "bcopy",
				.res_size = arg2,
				.res_id = fn_argument(proc, 1),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "bzero") == 0) {
		arg1 = fn_argument(proc, 1);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "bzero",
				.res_size = arg1,
				.res_id = fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strdup") == 0) {
		int len = trace_mem_readstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strdup",
				.res_size = len,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strndup") == 0) {
		arg1 = fn_argument(proc, 1);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strndup",
				.res_size = arg1,
				.res_id = (pointer_t)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strdupa") == 0) {
		int len = trace_mem_readstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strdupa",
				.res_size = len,
				.res_id = (pointer_t)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "strndupa") == 0) {
		arg1 = fn_argument(proc, 1);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "strndupa",
				.res_size = arg1,
				.res_id = (pointer_t)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wmemcpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wmemcpy",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wmempcpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wmempcpy",
				.res_size = arg2*WSIZE,
				.res_id = fn_argument(proc, 0)
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wmemmove") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wmemmove",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wmemset") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wmemset",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcscpy") == 0) {
		int len = trace_mem_readwstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcscpy",
				.res_size = len*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcsncpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcsncpy",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcpcpy") == 0) {
		arg1 = fn_argument(proc, 1);
		int len = trace_mem_readwstr(proc, arg1, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcpcpy",
				.res_size = len*WSIZE,
				.res_id = fn_argument(proc, 0)
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcpncpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcpncpy",
				.res_size = arg2*WSIZE,
				.res_id = fn_argument(proc, 0)
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcscat") == 0) {
		int len = trace_mem_readwstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcscat",
				.res_size = len*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcsncat") == 0) {
		size_t arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcsncat",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "wcsdup") == 0) {
		int len = trace_mem_readwstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcsdup",
				.res_size = len*WSIZE,
				.res_id = (pointer_t)(void*)retval
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
}

static struct plg_symbol symbols[] = {
		{.name = "memcpy", .hit = 0},
		{.name = "mempcpy", .hit = 0},
		{.name = "memmove", .hit = 0},
		{.name = "memccpy", .hit = 0},
		{.name = "memset", .hit = 0},
		{.name = "strcpy", .hit = 0},
		{.name = "strncpy", .hit = 0},
		{.name = "stpcpy", .hit = 0},
		{.name = "stpncpy", .hit = 0},
		{.name = "strcat", .hit = 0},
		{.name = "strncat", .hit = 0},
		{.name = "bcopy", .hit = 0},
		{.name = "bzero", .hit = 0},
		{.name = "strdup", .hit = 0},
		{.name = "strndup", .hit = 0},
/*		{.name = "strdupa", .hit = 0},
		{.name = "strndupa", .hit = 0},
*/
		{.name = "wmemcpy", .hit = 0},
		{.name = "wmempcpy", .hit = 0},
		{.name = "wmemmove", .hit = 0},
		{.name = "wmemset", .hit = 0},
		{.name = "wcscpy", .hit = 0},
		{.name = "wcsncpy", .hit = 0},
		{.name = "wcpcpy", .hit = 0},
		{.name = "wcpncpy", .hit = 0},
		{.name = "wcscat", .hit = 0},
		{.name = "wcsncat", .hit = 0},
		{.name = "wcsdup", .hit = 0},
};

static int get_symbols(struct plg_symbol **syms)
{
	*syms = symbols;
	return ARRAY_SIZE(symbols);
}


static void memtransfer_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_memory);
}

struct plg_api *init(void)
{
	static struct plg_api ma = {
		.api_version = memtransfer_api_version,
		.function_exit = memtransfer_function_exit,
		.get_symbols = get_symbols,
		.report_init = memtransfer_report_init,
	};
	return &ma;
}
