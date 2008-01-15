#include <stdio.h>
#include <stdlib.h>
#include <argp.h>

#include "options.h"
#include "report.h"

/* FIXME: Remove */
struct opt_x_t *opt_x = NULL;

struct arguments arguments;
char *library[MAX_LIBRARY];
int library_num = 0;

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
	{"debug", 'd', 0, 0, "maximum debug level", 0},
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
	struct arguments *arguments = state->input;

	switch (key) {
	case 'p':
		argp_error(state, "Warning: -p option no implemented");
		arguments->pid = atoi(arg);
		if (arguments->pid <= 0)
			argp_error(state, "PID must be positive");
		break;
	case 'n':
		arguments->nalloc = atoi(arg);
		if (arguments->nalloc <= 0 || arguments->nalloc > MAX_NALLOC)
			argp_error(state, "Number of allocations must be between 1 and %dK", MAX_NALLOC);
		break;
	case 't':
		arguments->depth = atoi(arg);
		if (arguments->depth <= 0 || arguments->depth > MAX_BT_DEPTH)
			argp_error(state, "Depth must be between 1 and %d", MAX_BT_DEPTH);
		break;
	case 'd':
		arguments->debug++;
		break;
	case 'e':
		argp_error(state, "Warning: -e option no implemented");
		break;
	case ARGP_KEY_END:
		if (arguments->pid == 0 && state->arg_num < 1)
			/* Not enough arguments. */
			argp_usage(state);
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

void process_options(int argc, char *argv[], int *remaining, struct arguments *arguments)
{
	/* Initial values */
	arguments->pid = 0;
	arguments->nalloc = MAX_NALLOC;
	arguments->depth = MAX_BT_DEPTH;
	arguments->debug = 0;

	/* parse and process arguments */

	argp_parse(&argp, argc, argv, 0, remaining, arguments);
}
