#ifndef FTK_MAPS_H
#define FTK_MAPS_H

#include <stdio.h>
#include <sys/types.h>

struct maps_data {
	FILE *fp;
	unsigned long lo, hi, off;
	char perm[4];
	unsigned int maj, min;
	int inum;
	char *path;
};

#define MAP_EXEC(md)	((md)->perm[2] == 'x')

extern int maps_init(struct maps_data *md, pid_t pid);
extern int maps_next(struct maps_data *md);
extern int maps_finish(struct maps_data *md);

#endif /* !FTK_MAPS_H */
