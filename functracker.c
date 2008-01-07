#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "event.h"
#include "options.h"
#include "ptrace.h"

pid_t execute_program(char *filename, char *argv[])
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		error_exit("fork");
	} else if (!pid) {	/* child */
		trace_me();
		execvp(filename, argv);
		fprintf(stderr, "could not execute \"%s\": %s\n", filename, strerror(errno));
		exit(1);
	}

	return pid;
}

int main(int argc, char *argv[])
{
	int prog_index;
	struct arguments arguments;

	memset(&arguments, 0, sizeof(struct arguments));
	process_options(argc, argv, &prog_index, &arguments);
	execute_program(argv[prog_index], argv + prog_index);

	return event_loop();
}
