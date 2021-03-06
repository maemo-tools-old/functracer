/*
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "arch-defs.h"
#include "config.h"
#include "options.h"
#include "report.h"
#include "backtrace.h"
#include "filter.h"

#define DEFAULT_BT_DEPTH		10

#define PLUGIN_DEFAULT	"memory"
#define PLUGIN_AUDIT 	"audit"

const char *argp_program_version = PACKAGE_STRING;

struct arguments arguments;

/* strings for arguments in help texts */
static const char args_doc[] = "[PROGRAM [ARGS...]]";

/* short description of program */
static const char doc[] = "Run PROGRAM and track selected functions.";

/* definitions of arguments for argp functions */
static const struct argp_option options[] = {
	{"pid", 'p', "PID", 0,
			"The process identifier to track.", 0},
	{"track", 'e', "PLUGIN", 0,
			"Name or path of plugin to use for tracing. 'memory' plugin is used by default.", 0},
	{"debug", 'd', NULL, 0,
			"Increase debug level. When configured with --enable-debug option it specifies "
			"debug message severity.", 0},
	{"no-time", 'T', NULL, 0,
			"Disable timestamps for all events.", 0},
	{"start", 's', NULL, 0,
			"Enable tracking immediately.", 0},
	{"backtrace-all", 'A', NULL, 0,
			"Enable backtrace reporting for all functions. By default only allocation function "
			"backtraces are reported (because de-allocation backtraces are less interesting and "
			"most often only add unnecessary overhead).", 0},
	{"backtrace-depth", 'b', "NUMBER", 0,
			"The maximum number of frames addresses in stack trace. 0 will disable "
			"backtracing functionality. Reducing backtrace depth improves performance, but gives "
			"less information of the function call origins.", 0},
	{"resolve-name", 'r', NULL, 0,
			"Enable symbol name resolution.", 0},
	{"output-dir",  'o', "DIR", 0,
			"The output directory for storing trace data. If output directory is not "
			"specified functracer dumps the data in standard output.", 0},
	{"audit", 'a', "SYMBOLS", 0,
			"Custom tracked symbol names list for audit module in format <symbol[;symbol...]>|@<filename>. "
			"In file the symbol names are separated by newlines.", 0},
	{"monitor", 'M', "SIZES", 0,
			"Monitor backtraces for resource allocations of the specified size, where SIZE is <size1>[,<size2>,...]", 0},
	{"library", 'L', "NAMES", 0,
			"Limit symbol scan only to the specified libraries, where NAMES is <library1>[,<library2>,...]. "
			"Speeds startup when auditing programs linking large libraries.", 0},
	{"skip-symbol-check", 'S', NULL, 0,
			"Skip abort when not all traced symbols are found from libraries directly linked by the binary. "
			"Needed when the symbols come from dlopen()ed libraries.", 0},
	{"quiet", 'q', NULL, 0,
			"Hide internal event messages.", 0},
	{"help", 'h', NULL, 0,
			"Give this help list.", -1},
	{"usage", OPT_USAGE, NULL, 0,
			"Give a short usage message.", -1},
	{"version", 'V', NULL, 0,
			"Print program version.", -1},
	{NULL, 0, NULL, 0, NULL, 0},
};

/* prototype for option handler */
static error_t parse_opt(int key, char *arg, struct argp_state *state);

/* data structure to communicate with argp functions */
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

/* handle program arguments */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arg_data = state->input;
	struct stat buf;

	switch (key) {
	case 'p':
		if (arg_data->npids >= MAX_NPIDS) {
			argp_error(state, "Maximum number of PID exceeded (%d)", MAX_NPIDS);
			return EINVAL;
		}
		arg_data->pid[arg_data->npids] = atoi(arg);
		if (arg_data->pid[arg_data->npids] <= 0) {
			argp_error(state, "invalid PID");
			return EINVAL;
		}
		arg_data->npids++;
		break;
	case 'b':
		arg_data->depth = atoi(arg);
		if (arg_data->depth < 0 || arg_data->depth > MAX_BT_DEPTH) {
			argp_error(state, "Depth must be between 0 and %d", MAX_BT_DEPTH);
			return EINVAL;
		}
		break;
	case 'd':
		arg_data->debug++;
		break;
	case 'e':
		arg_data->plugin = arg;
		break;
	case 'a':
		arg_data->audit = arg;
		arg_data->plugin = PLUGIN_AUDIT;
		break;
	case 'r':
		arg_data->resolve_name++;
		break;
	case 's':
		arg_data->enabled++;
		break;
	case 'A':
		arg_data->enable_free_bkt++;
		break;
	case 'h':
		argp_state_help(state, stdout,  ARGP_HELP_STD_HELP);
		/* Does not return. */
		break;
	case 'I':
		arg_data->time = 0;
		break;
	case 'o':
		arg_data->save_to_file++;
		arg_data->path = arg;
		if (stat(arg_data->path, &buf) || !S_ISDIR(buf.st_mode)) {
			argp_error(state, "Path must be a valid directory path");
		}
		if (access(arg_data->path, W_OK | X_OK) == -1) {
			argp_error(state, "No access to the output directory %s (%s)", arg, strerror(errno));
		}
		break;
	case 'q':
		arg_data->verbose = 0;
		break;
	case 'V':
		printf("%s\n", argp_program_version);
		exit(0);
		break;
	case OPT_USAGE:
		argp_state_help(state, stdout, ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK);
		/* Does not return. */
		break;
	case ARGP_KEY_END:
		if (!arg_data->npids && !state->arg_num) {
			/* Not enough arguments. */
			argp_usage(state);
		}
		break;
	case ARGP_KEY_ARGS:
		if (arg_data->npids) {
			/* Too many arguments */
			argp_usage(state);
		}
		arg_data->remaining_args = state->argv + state->next;
		break;
	case 'M':
		arg_data->filter_size = arg;
		break;
	case 'L':
		filter_initialize(arg);
		break;
	case 'S':
		arg_data->skip_symbol_check = true;
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

int process_options(int argc, char *argv[], int *remaining)
{
	error_t ret;

	/* Initial values */
	memset(&arguments, 0, sizeof(struct arguments));
	arguments.depth = MAX_BT_DEPTH > DEFAULT_BT_DEPTH ? DEFAULT_BT_DEPTH : MAX_BT_DEPTH;
	arguments.plugin = PLUGIN_DEFAULT;
	arguments.time = -1;
	arguments.verbose = 1;

	/* parse and process arguments */
	ret = argp_parse(&argp, argc, argv,
			ARGP_IN_ORDER | ARGP_NO_HELP, remaining, &arguments);

	/* create backtrace monitoring filter */
	arguments.filter = sp_rtrace_filter_create(arguments.enable_free_bkt ?
			SP_RTRACE_FILTER_TYPE_ALL : SP_RTRACE_FILTER_TYPE_ALLOC);
	sp_rtrace_filter_parse_size_opt(arguments.filter, arguments.filter_size);

	return ret;
}
