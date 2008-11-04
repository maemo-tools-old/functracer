/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
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

#include "callback.h"
#include "debug.h"
#include "function.h"
#include "process.h"
#include "report.h"
#include "target_mem.h"

#define MEM_API_VERSION "1.0"

char api_version[] = MEM_API_VERSION;

void function_exit(struct process *proc, const char *name)
{
	struct rp_alloc ra;

	addr_t retval = fn_return_value(proc);
	size_t arg0 = fn_argument(proc, 0);

	assert(proc->rp_data != NULL);
	if (strcmp(name, "__pthread_mutex_lock") == 0 ||
	    strcmp(name, "__pthread_mutex_unlock") == 0) {
		debug(3, "pthread function = %s\n", name);
		return;
	} else if (strcmp(name, "__libc_malloc") == 0) {
		ra.type = FN_MALLOC;
		ra.addr = retval;
		ra.size = arg0;
	} else if (strcmp(name, "__libc_calloc") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		ra.type = FN_CALLOC;
		ra.addr = retval;
		ra.nmemb = arg0;
		ra.size = arg1;
	} else if (strcmp(name, "__libc_memalign") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		ra.type = FN_MEMALIGN;
		ra.addr = retval;
		ra.boundary = arg0;
		ra.size = arg1;
	} else if (strcmp(name, "__libc_realloc") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		ra.type = FN_REALLOC;
		ra.addr = arg0;
		ra.addr_new = retval;
		ra.size = arg1;
	} else if (strcmp(name, "__libc_free") == 0 ) {
		/* Suppress "free(NULL)" calls from trace output. 
		 * They are a no-op according to ISO 
		 */
		if (arg0 == 0)
			return;
		ra.type = FN_FREE;
		ra.addr = arg0;
	} else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	rp_alloc(proc, &ra);
}

int library_match(const char *symname)
{
	return(strcmp(symname, "__pthread_mutex_lock") == 0 ||
	       strcmp(symname, "__pthread_mutex_unlock") == 0 ||
	       strcmp(symname, "__libc_calloc") == 0 ||
	       strcmp(symname, "__libc_malloc") == 0 ||
	       strcmp(symname, "__libc_realloc") == 0 ||
	       strcmp(symname, "__libc_free") == 0 ||
	       strcmp(symname, "__libc_memalign") == 0 ||
	       strcmp(symname, "__libc_valloc") == 0 ||
	       strcmp(symname, "__libc_pvalloc") == 0 ||
	       strcmp(symname, "posix_memalign") == 0);
}
