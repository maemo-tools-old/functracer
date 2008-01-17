#ifndef TT_OPTIONS_H
#define TT_OPTIONS_H

#include <sys/types.h>

struct arguments {
	char *args[2];		/* ARG1 & ARG2 */
	pid_t pid;
	int nalloc;
	int depth;
	int debug;
};

extern struct arguments arguments;

extern void process_options(int argc, char *argv[], int *remaining);

#endif /* TT_OPTIONS_H */
