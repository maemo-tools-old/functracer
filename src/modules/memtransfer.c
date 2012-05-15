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


static void write_function(struct process *proc, const char *name, size_t size, pointer_t id)
{
	struct rp_data *rd = proc->rp_data;
	assert(rd != NULL);
	sp_rtrace_fcall_t call = {
		.type = SP_RTRACE_FTYPE_ALLOC,
		.index = rd->rp_number,
		.context = context_mask,
		.timestamp = RP_TIMESTAMP,
		.name = name,
		.res_size = size,
		.res_id = id
	};
	sp_rtrace_print_call(rd->fp, &call);
	rp_write_backtraces(proc, &call);
	(rd->rp_number)++;
}

static void memtransfer_function_exit(struct process *proc, const char *name)
{
	int len;
	size_t arg1;
	addr_t retval = fn_return_value(proc);

	/* 8-bit char functions */
	if (strcmp(name, "memcpy") == 0) {
		write_function(proc, name, fn_argument(proc, 2), retval);
	} else if (strcmp(name, "mempcpy") == 0) {
		write_function(proc, name, fn_argument(proc, 2), fn_argument(proc, 0));
	} else if (strcmp(name, "memmove") == 0) {
		write_function(proc, name, fn_argument(proc, 2), retval);
	} else if (strcmp(name, "memccpy") == 0) {
		write_function(proc, name, fn_argument(proc, 3), fn_argument(proc, 0));
	} else if (strcmp(name, "memset") == 0) {
		write_function(proc, name, fn_argument(proc, 2), retval);
	} else if (strcmp(name, "strcpy") == 0) {
		len = trace_mem_readstr(proc, retval, NULL, 0);
		write_function(proc, name, len, retval);
	} else if (strcmp(name, "strncpy") == 0) {
		write_function(proc, name, fn_argument(proc, 2), retval);
	} else if (strcmp(name, "stpcpy") == 0) {
		len = trace_mem_readstr(proc, retval, NULL, 0);
		write_function(proc, name, len, fn_argument(proc, 0));
	} else if (strcmp(name, "stpncpy") == 0) {
		write_function(proc, name, fn_argument(proc, 2), fn_argument(proc, 0));
	} else if (strcmp(name, "strcat") == 0) {
		len = trace_mem_readstr(proc, retval, NULL, 0);
		write_function(proc, name, len, retval);
	} else if (strcmp(name, "strncat") == 0) {
		write_function(proc, name, fn_argument(proc, 2), retval);
	} else if (strcmp(name, "bcopy") == 0) {
		write_function(proc, name, fn_argument(proc, 2), fn_argument(proc, 1));
	} else if (strcmp(name, "bzero") == 0) {
		write_function(proc, name, fn_argument(proc, 1), fn_argument(proc, 0));
	} else if (strcmp(name, "strdup") == 0) {
		len = trace_mem_readstr(proc, retval, NULL, 0);
		write_function(proc, name, len, retval);
	} else if (strcmp(name, "strndup") == 0) {
		write_function(proc, name, fn_argument(proc, 1), retval);
	} else if (strcmp(name, "strdupa") == 0) {
		len = trace_mem_readstr(proc, retval, NULL, 0);
		write_function(proc, name, len, retval);
	} else if (strcmp(name, "strndupa") == 0) {
		write_function(proc, name, fn_argument(proc, 1), retval);

	/* wide character functions */
	} else if (strcmp(name, "wmemcpy") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), retval);
	} else if (strcmp(name, "wmempcpy") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), fn_argument(proc, 0));
	} else if (strcmp(name, "wmemmove") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), retval);
	} else if (strcmp(name, "wmemset") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), retval);
	} else if (strcmp(name, "wcscpy") == 0) {
		len = trace_mem_readwstr(proc, retval, NULL, 0);
		write_function(proc, name, WSIZE * len, retval);
	} else if (strcmp(name, "wcsncpy") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), retval);
	} else if (strcmp(name, "wcpcpy") == 0) {
		arg1 = fn_argument(proc, 1);
		len = trace_mem_readwstr(proc, arg1, NULL, 0);
		write_function(proc, name, WSIZE * len, fn_argument(proc, 0));
	} else if (strcmp(name, "wcpncpy") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), fn_argument(proc, 0));
	} else if (strcmp(name, "wcscat") == 0) {
		len = trace_mem_readwstr(proc, retval, NULL, 0);
		write_function(proc, name, WSIZE * len, retval);
	} else if (strcmp(name, "wcsncat") == 0) {
		write_function(proc, name, WSIZE * fn_argument(proc, 2), retval);
	} else if (strcmp(name, "wcsdup") == 0) {
		len = trace_mem_readwstr(proc, retval, NULL, 0);
		write_function(proc, name, WSIZE * len, retval);
	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
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
