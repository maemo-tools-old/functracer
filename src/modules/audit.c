/*
 * This is a functracer module used to track custom symbol list, specified
 * with functracer -a command line option.
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_tracker.h>
#include <sp_rtrace_defs.h>


#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "context.h"

#define AUDIT_API_VERSION "2.0"

#define RES_SIZE 	1
#define RES_ID		1

static char audit_api_version[] = AUDIT_API_VERSION;

static sp_rtrace_resource_t res_audit = {
		.id = 1,
		.type = "virtual",
		.desc = "Virtual resource for custom symbol tracking",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};

static sp_rtrace_tracker_t tracker;

static void audit_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;

	assert(proc->rp_data != NULL);
	
	const char* symname = sp_rtrace_tracker_query_symbol(&tracker, name);

	if (symname) {
		sp_rtrace_fcall_t call = {
			.type = SP_RTRACE_FTYPE_ALLOC,
			.index = rd->rp_number++,
			.context = context_mask,
			.timestamp = RP_TIMESTAMP,
			.name = (char*)symname,
			.res_size = RES_SIZE,
			.res_id = (pointer_t)RES_ID,
		};
		sp_rtrace_print_call(rd->fp, &call);
		free(symname);

	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	rp_write_backtraces(proc);
}

static int audit_library_match(const char *symname)
{
	return sp_rtrace_tracker_query_symbol(&tracker, symname) != NULL;
}

static void audit_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_audit);
}

struct plg_api *init(void)
{

	sp_rtrace_tracker_init(&tracker, arguments.audit);

	static struct plg_api ma = {
		.api_version = audit_api_version,
		.function_exit = audit_function_exit,
		.library_match = audit_library_match,
		.report_init = audit_report_init,
	};
	return &ma;
}
