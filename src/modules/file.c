/*
 * file is functracer module used to track file descriptor/pointer usage.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2008-2012 by Nokia Corporation
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
#include <linux/fcntl.h>
#include <limits.h>
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

#define FILE_API_VERSION "2.0"
#define RES_SIZE 1

#define MAX_DETAILS_LEN 512
#define DETAILS_DESC    25
#define DETAILS_OPT     10
#define DETAILS_NAME    (MAX_DETAILS_LEN - DETAILS_DESC - DETAILS_OPT)

static sp_rtrace_resource_t res_fd = {
		.id = 1,
		.type = "fd",
		.desc = "file descriptor",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};

static sp_rtrace_resource_t res_fp = {
		.id = 2,
		.type = "fp",
		.desc = "file pointer",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};


static char file_api_version[] = FILE_API_VERSION;


static void write_function(struct process *proc, const char *name, unsigned int type, sp_rtrace_farg_t *args, void *res_type, size_t size, pointer_t id)
{
	struct rp_data *rd = proc->rp_data;
	sp_rtrace_fcall_t call = {
		.type = type,
		.index = rd->rp_number,
		.context = context_mask,
		.timestamp = RP_TIMESTAMP,
		.name = name,
		.res_size = size,
		.res_id = id,
		.res_type = res_type,
		.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
	};
	sp_rtrace_print_call(rd->fp, &call);
	if (args) {
		sp_rtrace_print_args(rd->fp, args);
	}
	rp_write_backtraces(proc, &call);
	(rd->rp_number)++;
}

static void file_function_exit(struct process *proc, const char *name)
{
	char path[PATH_MAX], mode[16];

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);

	/* FILE* functions */

	if (strcmp(name, "_IO_fopen") == 0) {
		if (retval) {
			trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
			trace_mem_readstr(proc, fn_argument(proc, 1), mode, sizeof(mode));
			sp_rtrace_farg_t args[] = {
				{.name = "path", .value = path},
				{.name = "mode", .value = mode},
				{.name = NULL}
			};
			write_function(proc, "fopen", SP_RTRACE_FTYPE_ALLOC, args, res_fp.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "_IO_fdopen") == 0) {
		if (retval) {
			size_t arg0 = fn_argument(proc, 0);
			write_function(proc, "fdopen", SP_RTRACE_FTYPE_FREE, NULL, res_fd.type, 0, arg0);

			char fd[16];
			snprintf(fd, sizeof(fd), "%d", arg0);
			trace_mem_readstr(proc, fn_argument(proc, 1), mode, sizeof(mode));
			sp_rtrace_farg_t args[] = {
				{.name = "fd", .value = fd},
				{.name = "mode", .value = mode},
				{.name = NULL}
			};
			write_function(proc, "fdopen", SP_RTRACE_FTYPE_ALLOC, args, res_fp.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "freopen") == 0) {
		size_t arg2 = fn_argument(proc, 2);
		write_function(proc, "freopen", SP_RTRACE_FTYPE_FREE, NULL, res_fp.type, 0, arg2);
		if (retval) {
			trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
			trace_mem_readstr(proc, fn_argument(proc, 1), mode, sizeof(mode));
			sp_rtrace_farg_t args[] = {
				{.name = "path", .value = path},
				{.name = "mode", .value = mode},
				{.name = NULL}
			};
			write_function(proc, "freopen", SP_RTRACE_FTYPE_ALLOC, args, res_fp.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "_IO_fclose") == 0) {
		if (retval == 0) {
			size_t arg0 = fn_argument(proc, 0);
			write_function(proc, "fclose", SP_RTRACE_FTYPE_FREE, NULL, res_fp.type, 0, arg0);
		}

	} else if (strcmp(name, "fcloseall") == 0) {
		if (retval == 0) {
			write_function(proc, "fcloseall", SP_RTRACE_FTYPE_FREE, NULL, res_fp.type, 0, -1);
		}

	/* file descriptor functions */

	} else if (strcmp(name, "creat") == 0) {
		if (retval != (addr_t)-1) {
			trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
			snprintf(mode, sizeof(mode), "%ld", fn_argument(proc, 1));
			sp_rtrace_farg_t args[] = {
				{.name = "path", .value = path},
				{.name = "mode", .value = mode},
				{.name = NULL}
			};
			write_function(proc, "creat", SP_RTRACE_FTYPE_ALLOC, args, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "__open") == 0) {
		if (retval != (addr_t)-1) {
			trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
			snprintf(mode, sizeof(mode), "%ld", fn_argument(proc, 1));
			sp_rtrace_farg_t args[] = {
				{.name = "path", .value = path},
				{.name = "flags", .value = mode},
				{.name = NULL}
			};
			write_function(proc, "open", SP_RTRACE_FTYPE_ALLOC, args, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "__open64") == 0) {
		if (retval != (addr_t)-1) {
			trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
			snprintf(mode, sizeof(mode), "%ld", fn_argument(proc, 1));
			sp_rtrace_farg_t args[] = {
				{.name = "path", .value = path},
				{.name = "flags", .value = mode},
				{.name = NULL}
			};
			write_function(proc, "open64", SP_RTRACE_FTYPE_ALLOC, args, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "__close") == 0) {
		if (retval == 0) {
			size_t arg0 = fn_argument(proc, 0);
			write_function(proc, "close", SP_RTRACE_FTYPE_FREE, NULL, res_fd.type, 0, arg0);
		}

	} else if (strcmp(name, "dup") == 0) {
		if (retval != (addr_t)-1) {
			write_function(proc, "dup", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "__dup2") == 0) {
		if (retval != (addr_t)-1) {
			size_t arg1 = fn_argument(proc, 1);
			write_function(proc, "dup2", SP_RTRACE_FTYPE_FREE, NULL, res_fd.type, 0, arg1);
			write_function(proc, "dup2", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "fcntl") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		/* only these fcntl() operations create new FDs */
		if ( (arg1 == F_DUPFD || arg1 == F_DUPFD_CLOEXEC) && retval != (addr_t)-1) {
			size_t arg2 = fn_argument(proc, 2);
			write_function(proc, "fcntl", SP_RTRACE_FTYPE_FREE, NULL, res_fd.type, 0, arg2);
			write_function(proc, "fcntl", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "pipe") == 0) {
		if (retval != (addr_t)-1) {
			size_t fds[2];
			size_t arg0 = fn_argument(proc, 0);
			fds[0] = trace_mem_readw(proc, arg0);
			fds[1] = trace_mem_readw(proc, arg0 + 4);
			write_function(proc, "pipe", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, fds[0]);
			write_function(proc, "pipe", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, fds[1]);
		}

	} else if (strcmp(name, "pipe2") == 0) {
		if (retval != (addr_t)-1) {
			size_t fds[2];
			size_t arg0 = fn_argument(proc, 0);
			fds[0] = trace_mem_readw(proc, arg0);
			fds[1] = trace_mem_readw(proc, arg0 + 4);
			write_function(proc, "pipe2", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, fds[0]);
			write_function(proc, "pipe2", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, fds[1]);
		}

	/* socket file descriptor related functions */

	} else if (strcmp(name, "socket") == 0) {
		if (retval != (addr_t)-1) {
			char domain_s[64], type_s[64], protocol_s[64];
			snprintf(domain_s, sizeof(domain_s), "0x%lx", fn_argument(proc, 0));
			snprintf(type_s, sizeof(type_s), "0x%lx", fn_argument(proc, 1));
			snprintf(protocol_s, sizeof(protocol_s), "0x%lx", fn_argument(proc, 2));
			sp_rtrace_farg_t args[] = {
				{.name = "domain", .value = domain_s},
				{.name = "type", .value = type_s},
				{.name = "protocol", .value = protocol_s},
				{.name = NULL}
			};
			write_function(proc, "socket", SP_RTRACE_FTYPE_ALLOC, args, res_fd.type, RES_SIZE, retval);
		}

	} else if (strcmp(name, "socketpair") == 0){
		if (retval != (addr_t)-1) {
			char domain_s[64], type_s[64], protocol_s[64];
			snprintf(domain_s, sizeof(domain_s), "0x%lx", fn_argument(proc, 0));
			snprintf(type_s, sizeof(type_s), "0x%lx", fn_argument(proc, 1));
			snprintf(protocol_s, sizeof(protocol_s), "0x%lx", fn_argument(proc, 2));
			sp_rtrace_farg_t args[] = {
					{.name = "domain", .value = domain_s},
					{.name = "type", .value = type_s},
					{.name = "protocol", .value = protocol_s},
					{.name = NULL}
			};
			size_t fds[2];
			size_t arg3 = fn_argument(proc, 3);
			fds[0] = trace_mem_readw(proc, arg3);
			fds[1] = trace_mem_readw(proc, arg3+4);
			write_function(proc, "socketpair", SP_RTRACE_FTYPE_ALLOC, args, res_fd.type, RES_SIZE, fds[0]);
			write_function(proc, "socketpair", SP_RTRACE_FTYPE_ALLOC, args, res_fd.type, RES_SIZE, fds[1]);
		}

	} else if (strcmp(name, "accept") == 0) {
		if (retval != (addr_t)-1) {
			write_function(proc, "accept", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, retval);
		}

	/* other special file descriptor related functions */

	} else if (strcmp(name, "inotify_init") == 0) {
		if (retval != (addr_t)-1) {
			write_function(proc, "inotify_init", SP_RTRACE_FTYPE_ALLOC, NULL, res_fd.type, RES_SIZE, retval);
		}

	 } else {
		msg_warn("unexpected function exit (%s)\n", name);
	 }
}


static struct plg_symbol symbols[] = {
	/* FILE* functions */
	{.name = "_IO_fopen", .hit = 0}, // fopen64?
	{.name = "_IO_fdopen", .hit = 0},
	{.name = "freopen", .hit = 0},
//	{.name = "popen", .hit = 0},
	{.name = "_IO_fclose", .hit = 0},
	{.name = "fcloseall", .hit = 0},
	/* file descriptor functions */
	{.name = "creat", .hit = 0},	// creat64?
//	{.name = "openat", .hit = 0},	// openat64?
	{.name = "__open", .hit = 0},
	{.name = "__open64", .hit = 0},
//	{.name = "openat", .hit = 0},
	{.name = "__close", .hit = 0},
	{.name = "dup", .hit = 0},
	{.name = "__dup2", .hit = 0},
//	{.name = "dup3", .hit = 0},
	{.name = "fcntl", .hit = 0},
	{.name = "pipe", .hit = 0},
	{.name = "pipe2", .hit = 0},
	/* socket file descriptor functions */
//	{.name = "bind", .hit = 0},
//	{.name = "connect", .hit = 0},
	{.name = "accept", .hit = 0},
//	{.name = "accept4", .hit = 0},
	{.name = "socket", .hit = 0},
	{.name = "socketpair", .hit = 0},
	/* (other) special file descriptor functions */
//	{.name = "eventfd", .hit = 0},
//	{.name = "signalfd", .hit = 0},
//	{.name = "timerfd_create", .hit = 0},
//	{.name = "epoll_create", .hit = 0},
//	{.name = "epoll_create1", .hit = 0},
	{.name = "inotify_init", .hit = 0},
//	{.name = "inotify_init1", .hit = 0},
//	{.name = "posix_openpt", .hit = 0},
//	{.name = "getpt", .hit = 0},
};

static int get_symbols(struct plg_symbol **syms)
{
	*syms = symbols;
	return ARRAY_SIZE(symbols);
}


static void file_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_fd);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_fp);
}

struct plg_api *init(void)
{
	static struct plg_api ma = {
		.api_version = file_api_version,
		.function_exit = file_function_exit,
		.get_symbols = get_symbols,
		.report_init = file_report_init,
	};
	return &ma;
}
