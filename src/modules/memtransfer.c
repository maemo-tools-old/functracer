/*
 * memtransfer is functracer module used to track memory transfer operations.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2009-2010 by Nokia Corporation
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

#define WSIZE 4
#define POINTER 4
#define MEMTRANSFER_API_VERSION "2.0"

static char memtransfer_api_version[] = MEMTRANSFER_API_VERSION;

static sp_rtrace_resource_t res_memory = {
		.id = 1,
		.type = "memtransfer",
		.desc = "memory transfer operations in bytes",
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

	} else if (strcmp(name, "mempcpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "mempcpy",
				.res_size = arg2,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);

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

	} else if (strcmp(name, "memccpy") == 0) {
		arg3 = fn_argument(proc, 3);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "memccpy",
				.res_size = arg3,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);

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

	} else if (strcmp(name, "stpcpy") == 0) {
		int len = trace_mem_readstr(proc, retval, NULL, 0);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "stpcpy",
				.res_size = len,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "stpncpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "stpncpy",
				.res_size = arg2,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);

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

	} else if (strcmp(name, "bcopy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "bcopy",
				.res_size = arg2,
				.res_id = (pointer_t)fn_argument(proc, 1),
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "bzero") == 0) {
		arg1 = fn_argument(proc, 1);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "bzero",
				.res_size = arg1,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);

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

	} else if (strcmp(name, "wmempcpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wmempcpy",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)(void*)fn_argument(proc, 0)
		};
		sp_rtrace_print_call(rd->fp, &call);

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
				.res_id = (pointer_t)(void*)fn_argument(proc, 0)
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "wcpncpy") == 0) {
		arg2 = fn_argument(proc, 2);
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "wcpncpy",
				.res_size = arg2*WSIZE,
				.res_id = (pointer_t)(void*)fn_argument(proc, 0)
		};
		sp_rtrace_print_call(rd->fp, &call);

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

	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	rp_write_backtraces(proc);
}

static int memtransfer_library_match(const char *symname)
{
	return(strcmp(symname, "memcpy") == 0 ||
				 strcmp(symname, "mempcpy") == 0 ||
				 strcmp(symname, "memmove") == 0 ||
				 strcmp(symname, "memccpy") == 0 ||
				 strcmp(symname, "memset") == 0 ||
				 strcmp(symname, "strcpy") == 0 ||
				 strcmp(symname, "strncpy") == 0 ||
				 strcmp(symname, "stpcpy") == 0 ||
				 strcmp(symname, "stpncpy") == 0 ||
				 strcmp(symname, "strcat") == 0 ||
				 strcmp(symname, "strncat") == 0 ||
				 strcmp(symname, "bcopy") == 0 ||
				 strcmp(symname, "bzero") == 0 ||
				 strcmp(symname, "strdup") == 0 ||
				 strcmp(symname, "strndup") == 0 ||
				 strcmp(symname, "strdupa") == 0 ||
				 strcmp(symname, "strndupa") == 0 ||
				 strcmp(symname, "wmemcpy") == 0 ||
				 strcmp(symname, "wmempcpy") == 0 ||
				 strcmp(symname, "wmemmove") == 0 ||
				 strcmp(symname, "wmemset") == 0 ||
				 strcmp(symname, "wcscpy") == 0 ||
				 strcmp(symname, "wcsncpy") == 0 ||
				 strcmp(symname, "wcpcpy") == 0 ||
				 strcmp(symname, "wcpncpy") == 0 ||
				 strcmp(symname, "wcscat") == 0 ||
				 strcmp(symname, "wcsncat") == 0 ||
				 strcmp(symname, "wcsdup") == 0);
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
		.library_match = memtransfer_library_match,
		.report_init = memtransfer_report_init,
	};
	return &ma;
}
