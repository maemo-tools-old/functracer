#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

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
	{"depth", 't', "NUMBER", 0, "maximum backtrace depth", 0},
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
