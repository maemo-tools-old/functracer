/*
 * file is functracer module used to track file descriptor/pointer usage.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2008,2010 by Nokia Corporation
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

#define FILE_API_VERSION "2.0"
#define RES_SIZE 1

#define MAX_DETAILS_LEN 512
#define DETAILS_DESC    25
#define DETAILS_OPT     10
#define DETAILS_NAME    (MAX_DETAILS_LEN - DETAILS_DESC - DETAILS_OPT)

#define ARRAY_SIZE(x)   sizeof(x) / sizeof(x[0])

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


static void file_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	if (strcmp(name, "_IO_fopen") == 0) {
		if (retval == 0) return;
		else {
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "fopen",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fp.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);

			char path[PATH_MAX], mode[16];
			trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
			trace_mem_readstr(proc, fn_argument(proc, 1), mode, sizeof(mode));
			sp_rtrace_farg_t args[] = {
					{.name = "path", .value = path},
					{.name = "mode", .value = mode},
					{.name = NULL}
			};
			sp_rtrace_print_args(rd->fp, args);
		}
	} else if (strcmp(name, "_IO_fclose") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		is_free = 1;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "fclose",
				.res_size = 0,
				.res_id = (pointer_t)arg0,
				.res_type = (void*)res_fp.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "__open") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "open",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);

			char pathname[PATH_MAX], flags[16];
			trace_mem_readstr(proc, fn_argument(proc, 0), pathname, sizeof(pathname));
			snprintf(flags, sizeof(flags), "%ld", fn_argument(proc, 1));
			sp_rtrace_farg_t args[] = {
					{.name = "pathname", .value = pathname},
					{.name = "flags", .value = flags},
					{.name = NULL}
			};
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "__open64") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "open64",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);

			char pathname[PATH_MAX], flags[16];
			trace_mem_readstr(proc, fn_argument(proc, 0), pathname, sizeof(pathname));
			snprintf(flags, sizeof(flags), "%ld", fn_argument(proc, 1));
			sp_rtrace_farg_t args[] = {
					{.name = "pathname", .value = pathname},
					{.name = "flags", .value = flags},
					{.name = NULL}
			};
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "creat") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "creat",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);

			char pathname[PATH_MAX], mode[16];
			trace_mem_readstr(proc, fn_argument(proc, 0), pathname, sizeof(pathname));
			snprintf(mode, sizeof(mode), "0x%lx", fn_argument(proc, 1));
			sp_rtrace_farg_t args[] = {
					{.name = "pathname", .value = pathname},
					{.name = "mode", .value = mode},
					{.name = NULL}
			};
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "__close") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		is_free = 1;
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "close",
				.res_size = 0,
				.res_id = (pointer_t)arg0,
				.res_type = (void*)res_fd.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "fcloseall") == 0) {
		is_free = 1;
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "fcloseall",
				.res_size = 0,
				.res_id = (pointer_t)(-1),
				.res_type = (void*)res_fp.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "freopen") == 0) {
		sp_rtrace_fcall_t call1 = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number++,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "freopen",
				.res_size = 0,
				.res_id = (pointer_t)fn_argument(proc, 2),
				.res_type = (void*)res_fp.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call1);


		if (arguments.enable_free_bkt) rp_write_backtraces(proc);
		else sp_rtrace_print_comment(rd->fp, "\n"); 

		if (retval == 0) return;

		sp_rtrace_fcall_t call2 = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "freopen",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)retval,
				.res_type = (void*)res_fp.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call2);

		char path[PATH_MAX], mode[16];
		trace_mem_readstr(proc, fn_argument(proc, 0), path, sizeof(path));
		trace_mem_readstr(proc, fn_argument(proc, 1), mode, sizeof(mode));
		sp_rtrace_farg_t args[] = {
				{.name = "path", .value = path},
				{.name = "mode", .value = mode},
				{.name = NULL}
		};
		sp_rtrace_print_args(rd->fp, args);

	} else if (strcmp(name, "_IO_fdopen") == 0) {
		size_t filedes = fn_argument(proc, 0);
		sp_rtrace_fcall_t call1 = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number++,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "fdopen",
				.res_size = 0,
				.res_id = (pointer_t)filedes,
				.res_type = (void*)res_fd.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call1);

		if (arguments.enable_free_bkt) rp_write_backtraces(proc);
		else sp_rtrace_print_comment(rd->fp, "\n"); 
		
		if (retval == 0) return;

		sp_rtrace_fcall_t call2 = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "fdopen",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)retval,
				.res_type = (void*)res_fp.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call2);

		char fd[16], mode[16];
		snprintf(fd, sizeof(fd), "%d", filedes);
		trace_mem_readstr(proc, fn_argument(proc, 1), mode, sizeof(mode));
		sp_rtrace_farg_t args[] = {
				{.name = "fd", .value = fd},
				{.name = "flags", .value = mode},
				{.name = NULL}
		};
		sp_rtrace_print_args(rd->fp, args);

	} else if (strcmp(name, "socket") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "socket",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);

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
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "accept") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "accept",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);
		}

	} else if (strcmp(name, "__dup2") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			sp_rtrace_fcall_t call1 = {
					.type = SP_RTRACE_FTYPE_FREE,
					.index = rd->rp_number++,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "dup2",
					.res_size = 0,
					.res_id = (pointer_t)fn_argument(proc, 1),
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call1);

			if (arguments.enable_free_bkt) rp_write_backtraces(proc);
			else sp_rtrace_print_comment(rd->fp, "\n"); 
			
			sp_rtrace_fcall_t call2 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "dup2",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call2);
		}

	} else if (strcmp(name, "dup") == 0) {
		if (retval == (addr_t)-1) return;
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "dup",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)retval,
				.res_type = (void*)res_fd.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

	} else if (strcmp(name, "socketpair") == 0){
		if (retval == (addr_t)-1) return;
		else {
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

			size_t filedes[2];
			size_t filedes_addr = fn_argument(proc, 3);
			filedes[0] = trace_mem_readw(proc, filedes_addr);
			filedes[1] = trace_mem_readw(proc, filedes_addr+4);

			sp_rtrace_fcall_t call1 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number++,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "socketpair",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)filedes[0],
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call1);
			sp_rtrace_print_args(rd->fp, args);
			rp_write_backtraces(proc);

			sp_rtrace_fcall_t call2 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "socketpair",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)filedes[1],
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call2);
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "pipe") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			size_t arg0 = fn_argument(proc, 0);
			int fd1 = trace_mem_readw(proc, arg0);
			int fd2 = trace_mem_readw(proc, arg0 + 4);

			sp_rtrace_fcall_t call1 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number++,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "pipe",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)fd1,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call1);
			rp_write_backtraces(proc);

			sp_rtrace_fcall_t call2 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "pipe",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)fd2,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call2);
		}

	} else if (strcmp(name, "pipe2") == 0) {
		if (retval == (addr_t)-1) return;
		else {
			size_t arg0 = fn_argument(proc, 0);
			int fd1 = trace_mem_readw(proc, arg0);
			int fd2 = trace_mem_readw(proc, arg0 + 4);

			sp_rtrace_fcall_t call1 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number++,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "pipe2",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)fd1,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call1);
			rp_write_backtraces(proc);

			sp_rtrace_fcall_t call2 = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "pipe2",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)fd2,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call2);
		}

	} else if (strcmp(name, "fcntl") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		if ( (arg1 == F_DUPFD || arg1 == F_DUPFD_CLOEXEC) && retval != (addr_t)-1){
			sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_ALLOC,
					.index = rd->rp_number,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.name = "fcntl",
					.res_size = RES_SIZE,
					.res_id = (pointer_t)retval,
					.res_type = (void*)res_fd.type,
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			};
			sp_rtrace_print_call(rd->fp, &call);
		}
		else {
			/* not handling the rest of the fcntl commands */
			return;
		}

	} else if (strcmp(name, "inotify_init") == 0) {
		if (retval == (addr_t)-1) return;
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "inotify_init",
				.res_size = RES_SIZE,
				.res_id = (pointer_t)retval,
				.res_type = (void*)res_fd.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

	 } else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	if (!is_free || arguments.enable_free_bkt) {
		rp_write_backtraces(proc);
	}
	else {
		sp_rtrace_print_comment(rd->fp, "\n"); 
	}
}

static int file_library_match(const char *symname)
{
	return(strcmp(symname, "_IO_fopen") == 0 ||
				strcmp(symname, "_IO_fclose") == 0 ||
				strcmp(symname, "__open") == 0 ||
				strcmp(symname, "__open64") == 0 ||
				strcmp(symname, "__close") == 0 ||
				strcmp(symname, "fcloseall") == 0 ||
				strcmp(symname, "creat") == 0 ||
				strcmp(symname, "freopen") == 0 ||
				strcmp(symname, "_IO_fdopen") == 0 ||
				strcmp(symname, "accept") == 0 ||
				strcmp(symname, "dup") == 0 ||
				strcmp(symname, "__dup2") == 0 ||
				strcmp(symname, "socket") == 0 ||
				strcmp(symname, "fcntl") == 0 ||
				strcmp(symname, "socketpair") == 0 ||
				strcmp(symname, "inotify_init") == 0 ||
				strcmp(symname, "pipe") == 0 ||
				strcmp(symname, "pipe2") == 0
				);
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
		.library_match = file_library_match,
		.report_init = file_report_init,
	};
	return &ma;
}
