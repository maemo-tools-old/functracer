/*
 * thread-resource is functracer module used to keep track on memory
 * allocation/release caused by creating/joining/detaching threads.
 *
 * Warning, the code used to determine thread attributes passed to
 * pthread_create relies on the pthread_attr_t structure internal
 * implementation.
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
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

#define THREAD_API_VERSION "2.0"
#define RES_SIZE 1

static char thread_api_version[] = THREAD_API_VERSION;

static sp_rtrace_resource_t res_thread = {
		.id = 1,
		.type = "pthread_t",
		.desc = "posix thread",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};


static void thread_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	
	/* TODO: finalize process tracking
	if (strcmp(name, "system") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "system", RES_SIZE, retval);

	} else if (strcmp(name, "__fork") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "fork", RES_SIZE, retval);

	} else if (strcmp(name, "__vfork") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "vfork", RES_SIZE, retval);

	} else if (strcmp(name, "clone") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "clone", RES_SIZE, retval);

	} else if (strcmp(name, "execv") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "execv", RES_SIZE, retval);

	} else if (strcmp(name, "execl") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "execl", RES_SIZE, retval);

	} else if (strcmp(name, "execve") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "execve", RES_SIZE, retval);

	} else if (strcmp(name, "execle") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "execle", RES_SIZE, retval);

	} else if (strcmp(name, "execvp") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "execvp", RES_SIZE, retval);

	} else if (strcmp(name, "execlp") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "execlp", RES_SIZE, retval);

	} else if (strcmp(name, "waitpid") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "waitpid", RES_SIZE, retval);

	} else if (strcmp(name, "wait") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "wait", RES_SIZE, retval);

	} else if (strcmp(name, "wait4") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "wait4", RES_SIZE, retval);

	} else if (strcmp(name, "wait3") == 0) {
		sp_rtrace_print_call(rd->rp, rd->rp_number, 0, -1, "wait3", RES_SIZE, retval);

	} else
	*/
	if (strcmp(name, "pthread_join") == 0) {
		if (retval != 0) {
			/* failures doesn't allocate resources - skip */
			return;
		}

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "pthread_join",
				.res_size = 0,
				.res_id = (pointer_t)fn_argument(proc, 0),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "pthread_create") == 0) {
		if (retval != 0) {
			/* failures doesn't allocate resources - skip */
			return;
		}
		int attr_addr = fn_argument(proc, 1);
		int state = PTHREAD_CREATE_JOINABLE;
		if (attr_addr) {
			pthread_attr_t attr;
			trace_mem_read(proc, attr_addr, &attr, sizeof(pthread_attr_t));
			pthread_attr_getdetachstate(&attr, &state);
		}
		/* track only joinable threads */
		if (state == PTHREAD_CREATE_DETACHED) {
			return;
		}

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "pthread_create",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)trace_mem_readw(proc, fn_argument(proc, 0)),
		};
		sp_rtrace_print_call(rd->fp, &call);
		rp_write_backtraces(proc, &call);

	} else if (strcmp(name, "pthread_detach") == 0) {
		if (retval != 0) {
			/* failures doesn't allocate resources - skip */
			return;
		}
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "pthread_detach",
				.res_size = 0,
				.res_id = (pointer_t)fn_argument(proc, 0),
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
		{.name = "pthread_join", .hit = 0},
		{.name = "pthread_create", .hit = 0},
		{.name = "pthread_detach", .hit = 0},
};

static int get_symbols(struct plg_symbol **syms)
{
	*syms = symbols;
	return ARRAY_SIZE(symbols);
}

static void thread_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_thread);
}

struct plg_api *init(void)
{
	static struct plg_api ma = {
		.api_version = thread_api_version,
		.function_exit = thread_function_exit,
		.get_symbols = get_symbols,
		.report_init = thread_report_init,
	};
	return &ma;
}
