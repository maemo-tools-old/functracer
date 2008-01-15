#ifndef TT_OPTIONS_H
#define TT_OPTIONS_H

#include <sys/types.h>

struct opt_x_t {
	char *name;
	int found;
	struct opt_x_t *next;
};
extern struct opt_x_t *opt_x;	/* list of functions to break at */

struct arguments {
	char *args[2];		/* ARG1 & ARG2 */
	pid_t pid;
	int nalloc;
	int depth;
	int debug;
};

#define MAX_LIBRARY     30
extern struct arguments arguments;
extern int library_num;
extern char *library[MAX_LIBRARY];

extern void process_options(int argc, char *argv[], int *remaining,
			    struct arguments *arguments);

#endif /* TT_OPTIONS_H */
