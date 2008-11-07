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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "options.h"
#include "config.h"
#include "report.h"
#include "backtrace.h"

const char *argp_program_version = PACKAGE_STRING;

struct arguments arguments;

/* strings for arguments in help texts */
static const char args_doc[] = "PROGRAM [ARGS...]";

/* short description of program */
static const char doc[] = "Run PROGRAM and track selected functions.";

/* definitions of arguments for argp functions */
static const struct argp_option options[] = {
	{"pid", 'p', "PID", 0, "which PID to track", 0},
	{"track", 'e', "PLUGIN", 0,
	 "which set of functions to track", 0},
	{"debug", 'd', NULL, 0, "maximum debug level", 0},
	{"start", 's', NULL, 0, "enable tracking memory from beginning", 0},
	{"free-backtraces", 'b', NULL, 0, 
	 "enable backtraces for free() function", 0},
	{"depth", 't', "NUMBER", 0, "maximum backtrace depth", 0},
	{"resolve-name", 'r', NULL, 0, "enable symbol name resolution", 0},
	{"file",  'f', NULL, 0,
	 "use a file to save backtraces instead of dump to stdout", 0},
	{"path",  'l', "DIR", 0,
         "dump reports to a custom location (defaults to homedir)", 0},
	{"help", 'h', NULL, 0, "Give this help list", -1},
	{"usage", OPT_USAGE, NULL, 0, "Give a short usage message", -1},
	{"version", 'V', NULL, 0, "Print program version", -1},
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
	case 't':
		arg_data->depth = atoi(arg);
		if (arg_data->depth <= 0 || arg_data->depth > MAX_BT_DEPTH) {
			argp_error(state, "Depth must be between 1 and %d", MAX_BT_DEPTH);
			return EINVAL;
		}
		break;
	case 'd':
		arg_data->debug++;
		break;
	case 'e':
		arg_data->plugin = arg;
		break;
	case 'r':
		arg_data->resolve_name++;
		break;
	case 's':
		arg_data->enabled++;
		break;
	case 'b':
		arg_data->enable_free_bkt++;
		break;
	case 'f':
		arg_data->save_to_file++;
		break;
	case 'h':
		argp_state_help(state, stdout,  ARGP_HELP_STD_HELP);
		/* Does not return. */
		break;
	case 'l':
		arg_data->path = arg;
		if (stat(arg_data->path, &buf) || !S_ISDIR(buf.st_mode)) {
			argp_error(state, "Path must be a valid directory path");
			return ENOENT;
		}
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
		if (arg_data->npids == 0) {
			/* Not enough arguments. */
			argp_usage(state);
		}
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
	arguments.depth = MAX_BT_DEPTH;
	arguments.plugin = "memory";

	/* parse and process arguments */
	ret = argp_parse(&argp, argc, argv,
			ARGP_IN_ORDER | ARGP_NO_HELP, remaining, &arguments);

	return ret;
}
