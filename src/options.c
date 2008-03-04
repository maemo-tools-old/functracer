/*
 * This file is part of Functracker.
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
#include "report.h"

struct arguments arguments;

/* strings for arguments in help texts */
static const char args_doc[] = "PROGRAM [ARGS...]";

/* short description of program */
static const char doc[] = "Run PROGRAM and track selected functions.";

/* definitions of arguments for argp functions */
static const struct argp_option options[] = {
	{"track-pid", 'p', "PID", 0, "which PID to track", 0},
	{"track-function", 'e', "FUNCTION", 0,
	 "which function to track (NOT IMPLEMENTED)", 0},
	{"alloc-number", 'n', "NUMBER", 0, "maximum number of allocations", 0},
	{"debug", 'd', NULL, 0, "maximum debug level", 0},
	{"start", 's', NULL, 0, "enable tracking memory from beginning", 0},
	{"depth", 't', "NUMBER", 0, "maximum backtrace depth", 0},
	{"file",  'f', NULL, 0,
	 "use a file to save backtraces instead of dump to stdout", 0},
	{"path",  'l', "DIR", 0,
         "dump reports to a custom location (defaults to homedir)", 0},
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
		if (arg_data->npids >= MAX_NPIDS)
			argp_error(state, "Maximum number of PID exceeded (%d)", MAX_NPIDS);
		arg_data->pid[arg_data->npids] = atoi(arg);
		if (arg_data->pid[arg_data->npids] <= 0)
			argp_error(state, "invalid PID");
		arg_data->npids++;
		break;
	case 'n':
		arg_data->nalloc = atoi(arg);
		if (arg_data->nalloc <= 0 || arg_data->nalloc > MAX_NALLOC)
			argp_error(state, "Number of allocations must be between 1 and %dK", MAX_NALLOC);
		break;
	case 't':
		arg_data->depth = atoi(arg);
		if (arg_data->depth <= 0 || arg_data->depth > MAX_BT_DEPTH)
			argp_error(state, "Depth must be between 1 and %d", MAX_BT_DEPTH);
		break;
	case 'd':
		arg_data->debug++;
		break;
	case 's':
		arg_data->enabled++;
		break;
	case 'f':
		arg_data->save_to_file++;
		break;
	case 'l':
		arg_data->path = arg;
		stat(arg_data->path, &buf);
		if (!S_ISDIR(buf.st_mode))
			argp_error(state, "Path must be a valid directory path");
		break;
	case ARGP_KEY_END:
		if (arg_data->npids == 0 && state->arg_num < 1)
			/* Not enough arguments. */
			argp_usage(state);
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

void process_options(int argc, char *argv[], int *remaining)
{
	/* Initial values */
	memset(&arguments, 0, sizeof(struct arguments));
	arguments.nalloc = MAX_NALLOC;
	arguments.depth = MAX_BT_DEPTH;

	/* parse and process arguments */
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, remaining, &arguments);
}
