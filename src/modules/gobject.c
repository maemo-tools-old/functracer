/*
 * gobject is functracer module used to track GObject allocations
 * and releases.
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
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "context.h"

#define GOBJECT_API_VERSION "2.0"
#define RES_SIZE 4

static sp_rtrace_resource_t res_gobject = {
		.id = 1,
		.type = "gobject",
		.desc = "GObject instance",
		.flags = SP_RTRACE_RESOURCE_REFCOUNT,
};

static char gobject_api_version[] = GOBJECT_API_VERSION;

static void gobject_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	if (strcmp(name, "g_object_newv") == 0) {
		/* suppress allocation failure reports */
		if (retval == 0) return;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "g_object_newv",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)retval,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "g_object_ref") == 0) {
		/* suppress allocation failure reports */
		if (retval == 0) return;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "g_object_ref",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "g_object_unref") == 0) {
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "g_object_unref",
				.res_size = 0,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	rp_write_backtraces(proc);
}

static int gobject_library_match(const char *symname)
{
	return( 	strcmp(symname, "g_object_newv") == 0 ||
			strcmp(symname, "g_object_ref") == 0 ||
			strcmp(symname, "g_object_unref") == 0);
}

static void gobject_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_gobject);
}

struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = gobject_api_version,
		.function_exit = gobject_function_exit,
		.library_match = gobject_library_match,
		.report_init = gobject_report_init,
	};
	return &ma;
}
