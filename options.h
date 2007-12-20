#ifndef TT_OPTIONS_H
#define TT_OPTIONS_H

struct opt_x_t {
	char *name;
	int found;
	struct opt_x_t *next;
};
extern struct opt_x_t *opt_x;   /* list of functions to break at */

#define MAX_LIBRARY     30
extern int library_num;
extern char *library[MAX_LIBRARY];

#endif /* TT_OPTIONS_H */
