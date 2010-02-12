/*
 * gobject is functracer module used to track GObject allocations
 * and releases.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2009 by Nokia Corporation
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

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"

#define GOBJECT_API_VERSION "2.0"
#define RES_SIZE 4

static char gobject_api_version[] = GOBJECT_API_VERSION;

static void gobject_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	if (strcmp(name, "g_object_newv") == 0) {
		/* suppress allocation failure reports */
		if (retval == 0) return;
		rp_alloc(proc, rd->rp_number, "g_object_newv", RES_SIZE, retval);

	} else if (strcmp(name, "g_object_ref") == 0) {
		/* suppress allocation failure reports */
		if (retval == 0) return;
		rp_alloc(proc, rd->rp_number, "g_object_ref", RES_SIZE, retval);

	} else if (strcmp(name, "g_object_unref") == 0) {
				size_t arg0 = fn_argument(proc, 0);
		rp_free(proc, rd->rp_number, "g_object_unref", arg0);

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

struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = gobject_api_version,
		.function_exit = gobject_function_exit,
		.library_match = gobject_library_match,
	};
	return &ma;
}
