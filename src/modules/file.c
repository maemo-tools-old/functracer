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
#include <sp_rtrace_formatter.h>

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"

#define FILE_API_VERSION "2.0"
#define RES_SIZE 1

#define MAX_DETAILS_LEN 512
#define DETAILS_DESC    25
#define DETAILS_OPT     10
#define DETAILS_NAME    (MAX_DETAILS_LEN - DETAILS_DESC - DETAILS_OPT)

#define ARRAY_SIZE(x)   sizeof(x) / sizeof(x[0])

static char file_api_version[] = FILE_API_VERSION;

/**
 * strcat_arg_name function appends argument name to the specified buffer.
 *
 * @param buffer[out]   the output buffer.
 * @param size[in]      the output buffer size.
 * @param name[in]      the argument name to append to the buffer.
 * @param length[in]    the argument name length (not including the terminating null character).
 * @return              the number of appended characters (not including the terminating null character).
 */
static int strcat_arg_name(char* buffer, int size, const char* name, int length) {
	if (name) {
		strncat(buffer, name, size);
	}
	return length < size ? length : size - 1;
}

/**
 * strcat_arg_str function appends zero terminated string argument name and value to the output buffer.
 *
 * @param buffer[out]     the output buffer.
 * @param size[in]        the output buffer size.
 * @param name[in]        the argument name.
 * @param length[in]      the argument name length.
 * @param proc[in]        the process data.
 * @param n[in]           the argument index.
 * @return                the number of appended characters (not including the terminating null character).
 */
static int strcat_arg_str(char* buffer, int size, const char* name, int length, struct process* proc, int n) {
	int offset = strcat_arg_name(buffer, size, name, length);
	return trace_mem_readstr(proc, fn_argument(proc, n), buffer + offset, size) + offset;
}

/**
 * strcat_arg_str function appends integer argument name and value to the output buffer.
 *
 * @param buffer[out]     the output buffer.
 * @param size[in]        the output buffer size.
 * @param name[in]        the argument name.
 * @param length[in]      the argument name length.
 * @param proc[in]        the process data.
 * @param n[in]           the argument index.
 * @return                the number of appended characters (not including the terminating null character).
 */
static int strcat_arg_int(char* buffer, int size, const char* name, int length, struct process* proc, int n) {
	int offset = strcat_arg_name(buffer, size, name, length);
	return snprintf(buffer + offset, size, "%ld", fn_argument(proc, n)) + offset;
}

/**
 * strcat_arg_str function appends name and integer value to the output buffer.
 *
 * @param buffer[out]     the output buffer.
 * @param size[in]        the output buffer size.
 * @param name[in]        the argument name.
 * @param length[in]      the argument name length.
 * @param value[in]       the integer value.
 * @return                the number of appended characters (not including the terminating null character).
 */static int strcat_const_int(char* buffer, int size, const char* name, int length, int value) {
	int offset = strcat_arg_name(buffer, size, name, length);
	return snprintf(buffer + offset, size, "%d", value) + offset;
}

#define STRCAT_ARG_STR(buffer, size, name, proc, n)   strcat_arg_str(buffer, size, name, sizeof(name) - 1, proc, n)
#define STRCAT_ARG_INT(buffer, size, name, proc, n)   strcat_arg_int(buffer, size, name, sizeof(name) - 1, proc, n)
#define STRCAT_CONST_INT(buffer, size, name, value)   strcat_const_int(buffer, size, name, sizeof(name) - 1, value)

static void file_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	if (strcmp(name, "_IO_fopen") == 0) {
		if (retval == 0) return;
		else {
			char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "fopen", RES_SIZE, (void*)retval);

			*pargs = ptr + STRCAT_ARG_STR(ptr, MAX_DETAILS_LEN, "filename:", proc, 0) + 1;
			STRCAT_ARG_STR(*pargs, MAX_DETAILS_LEN - (*pargs - details), "opentype:", proc, 1);
			*++pargs = NULL;
			sp_rtrace_print_args(rd->fp, args);

		}
	} else if (strcmp(name, "_IO_fclose") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		is_free = 1;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "fclose", 0, (void*)arg0);

	} else if (strcmp(name, "__open") == 0) {
		if (retval == -1) return;
		else {
			char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "open", RES_SIZE, (void*)retval);

			*pargs = ptr + STRCAT_ARG_STR(ptr, MAX_DETAILS_LEN, "filename:", proc, 0) + 1;
			STRCAT_ARG_INT(*pargs, MAX_DETAILS_LEN - (*pargs - details), "flags:", proc, 1);

			*++pargs = NULL;
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "creat") == 0) {
		if (retval == -1) return;
		else {
			char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "creat", RES_SIZE, (void*)retval);

			*pargs = ptr + STRCAT_ARG_STR(ptr, MAX_DETAILS_LEN, "filename:", proc, 0) + 1;
			STRCAT_ARG_INT(*pargs, MAX_DETAILS_LEN - (*pargs - details), "flags:", proc, 1);
			*++pargs = NULL;
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "__close") == 0) {
		size_t arg0 = fn_argument(proc, 0);
		is_free = 1;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "close", 0, (void*)arg0);

	} else if (strcmp(name, "fcloseall") == 0) {
		is_free = 1;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "fcloseall", 0, (void*)0);

	} else if (strcmp(name, "freopen") == 0) {
		sp_rtrace_print_call(rd->fp, rd->rp_number++, 0, RP_TIMESTAMP, "close", 0, (void*)fn_argument(proc, 2));
		if (retval == 0) return;

		char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "freopen", RES_SIZE, (void*)retval);

		*pargs = ptr + STRCAT_ARG_STR(ptr, MAX_DETAILS_LEN, "filename:", proc, 0) + 1;
		STRCAT_ARG_STR(*pargs, MAX_DETAILS_LEN - (*pargs - details), "opentype:", proc, 1);
		*++pargs = NULL;
		sp_rtrace_print_args(rd->fp, args);

	} else if (strcmp(name, "_IO_fdopen") == 0) {
		size_t filedes = fn_argument(proc, 0);
		sp_rtrace_print_call(rd->fp, rd->rp_number++, 0, RP_TIMESTAMP, "fdopen", 0, (void*)filedes);
		if (retval == 0) return;

		char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "fdopen", RES_SIZE, (void*)retval);

		*pargs = ptr + STRCAT_CONST_INT(ptr, MAX_DETAILS_LEN, "descriptor:", filedes) + 1;
		STRCAT_ARG_STR(*pargs, MAX_DETAILS_LEN - (*pargs - details), "opentype:", proc, 1);
		*++pargs = NULL;
		sp_rtrace_print_args(rd->fp, args);

	} else if (strcmp(name, "socket") == 0) {
		if (retval == -1) return;
		else {
			char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "socket", RES_SIZE, (void*)retval);

			*pargs = ptr + sprintf(ptr, "namespace:%ld", fn_argument(proc, 0));
			ptr = *pargs++;
			*pargs = ptr + sprintf(ptr, "style:%ld", fn_argument(proc, 1));
			sprintf(*pargs, "protocol:%ld", fn_argument(proc, 2));
			*++pargs = NULL;
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "accept") == 0) {
		if (retval == -1) return;
		else {
			char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "accept", RES_SIZE, (void*)retval);

			sprintf(ptr, "socket:%ld", fn_argument(proc, 0));
			*++pargs = NULL;
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "__dup2") == 0) {
		if (retval == -1) return;
		else {
			sp_rtrace_print_call(rd->fp, rd->rp_number++, 0, RP_TIMESTAMP, "dup2", 0, (void*)fn_argument(proc, 1));
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "dup2", RES_SIZE, (void*)retval);
		}

	} else if (strcmp(name, "dup") == 0) {
		if (retval == -1) return;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "dup", RES_SIZE, (void*)retval);

	} else if (strcmp(name, "socketpair") == 0){
		if (retval == -1) return;
		else {
			char details[MAX_DETAILS_LEN] = "", *args[8] = {details}, **pargs = args, *ptr = *pargs++;

			*pargs = ptr + sprintf(ptr, "namespace:%ld", fn_argument(proc, 0));
			ptr = *pargs++;
			*pargs = ptr + sprintf(ptr, "style:%ld", fn_argument(proc, 1));
			sprintf(*pargs, "protocol:%ld", fn_argument(proc, 2));
			*++pargs = NULL;

			size_t filedes[2];
			size_t filedes_addr = fn_argument(proc, 3);
			filedes[0] = trace_mem_readw(proc, filedes_addr);
			filedes[1] = trace_mem_readw(proc, filedes_addr+4);

			sp_rtrace_print_call(rd->fp, rd->rp_number++, 0, RP_TIMESTAMP, "socketpair", RES_SIZE, (void*)filedes[0]);
			sp_rtrace_print_args(rd->fp, args);
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "socketpair", RES_SIZE, (void*)filedes[1]);
			sp_rtrace_print_args(rd->fp, args);
		}

	} else if (strcmp(name, "pipe") == 0) {
		if (retval == -1) return;
		else {
			size_t arg0 = fn_argument(proc, 0);
			int fd1 = trace_mem_readw(proc, arg0);
			int fd2 = trace_mem_readw(proc, arg0 + 4);
			sp_rtrace_print_call(rd->fp, rd->rp_number++, 0, RP_TIMESTAMP, "pipe", RES_SIZE, (void*)fd1);
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "pipe", RES_SIZE, (void*)fd2);
		}

	} else if (strcmp(name, "fcntl") == 0) {
		size_t arg1 = fn_argument(proc, 1);
		if ( (arg1 == F_DUPFD || arg1 == F_DUPFD_CLOEXEC) && retval != -1){
			sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "fcntl", RES_SIZE, (void*)retval);
		}
		else {
			/* not handling the rest of the fcntl commands */
			return;
		}

	} else if (strcmp(name, "inotify_init") == 0) {
		if (retval == -1) return;
		sp_rtrace_print_call(rd->fp, rd->rp_number, 0, RP_TIMESTAMP, "inotify_init", RES_SIZE, (void*)retval);

	 } else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	if (!is_free || arguments.enable_free_bkt) {
		rp_write_backtraces(proc);
	}
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
				strcmp(symname, "socket") == 0 ||
				strcmp(symname, "fcntl") == 0 ||
				strcmp(symname, "socketpair") == 0 ||
				strcmp(symname, "inotify_init") == 0 ||
				strcmp(symname, "pipe") == 0 );
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
