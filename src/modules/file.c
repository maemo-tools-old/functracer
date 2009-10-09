/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
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

#define FILE_API_VERSION "2.0"
#define RES_SIZE 1

static char file_api_version[] = FILE_API_VERSION;

static void file_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	if (strcmp(name, "_IO_fopen") == 0) {
		rp_alloc(proc, rd->rp_number, "fopen", RES_SIZE, retval); 

	} else if (strcmp(name, "_IO_fclose") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		is_free = 1;
		rp_free(proc, rd->rp_number, "fclose", arg0);

	} else if (strcmp(name, "__open") == 0) {
		rp_alloc(proc, rd->rp_number, "open", RES_SIZE, retval);

	} else if (strcmp(name, "creat") == 0) {
		rp_alloc(proc, rd->rp_number, "creat", RES_SIZE, retval);

	} else if (strcmp(name, "__close") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		is_free = 1;
		rp_free(proc, rd->rp_number, "close", arg0);

	} else if (strcmp(name, "fcloseall") == 0) {
		is_free = 1;
		rp_free(proc, rd->rp_number, "fcloseall", 0);

	} else if (strcmp(name, "freopen") == 0) {
		size_t arg0 = fn_argument(proc, 2);
		rp_free(proc, rd->rp_number++, "freopen", arg0);
		rp_alloc(proc, rd->rp_number, "freopen", RES_SIZE, retval);

	} else if (strcmp(name, "_IO_fdopen") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		rp_free(proc, rd->rp_number++, "fdopen", arg0);
		rp_alloc(proc, rd->rp_number, "fdopen", RES_SIZE, retval);

	} else if (strcmp(name, "socket") == 0) {
		rp_alloc(proc, rd->rp_number, "socket", RES_SIZE, retval);

	} else if (strcmp(name, "accept") == 0) {
		rp_alloc(proc, rd->rp_number, "accept", RES_SIZE, retval);

	} else if (strcmp(name, "__dup2") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		rp_free(proc, rd->rp_number++, "dup2", arg1);
		rp_alloc(proc, rd->rp_number, "dup2", RES_SIZE, retval);

	} else if (strcmp(name, "dup") == 0) {
		rp_alloc(proc, rd->rp_number, "dup", RES_SIZE, retval); 

	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	if (!is_free || arguments.enable_free_bkt)
		rp_write_backtraces(proc);
}

static int file_library_match(const char *symname)
{
	return(strcmp(symname, "_IO_fopen") == 0 ||
	       strcmp(symname, "_IO_fclose") == 0 ||
	       strcmp(symname, "__open") == 0 ||
	       strcmp(symname, "__close") == 0 ||
	       strcmp(symname, "fcloseall") == 0 ||
	       strcmp(symname, "creat") == 0 ||
	       strcmp(symname, "freopen") == 0 ||
	       strcmp(symname, "_IO_fdopen") == 0 ||
	       strcmp(symname, "accept") == 0 ||
	       strcmp(symname, "dup") == 0 ||
	       strcmp(symname, "__dup2") == 0 ||
	       strcmp(symname, "socket") == 0);

}

struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = file_api_version,
		.function_exit = file_function_exit,
		.library_match = file_library_match,
	};
	return &ma;
}
