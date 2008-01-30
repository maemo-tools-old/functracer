#ifndef TT_OPTIONS_H
#define TT_OPTIONS_H

#include <sys/types.h>

#define MAX_NPIDS 20

struct arguments {
	pid_t pid[MAX_NPIDS];
	int npids;
	int nalloc;
	int depth;
	int debug;
	int enabled;
};

extern struct arguments arguments;

extern void process_options(int argc, char *argv[], int *remaining);

#endif /* TT_OPTIONS_H */
