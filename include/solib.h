#ifndef FTK_SOLIB_H
#define FTK_SOLIB_H

#include "process.h"

struct solib_list {
	void *base_addr;
	char *path;
	struct solib_list *next;
};

struct library_symbol {
	char *name;
	void *enter_addr;
	struct library_symbol *next;
};

extern void solib_update_list(struct process *proc);
extern void *solib_dl_debug_address(struct process *proc);
extern struct library_symbol *solib_read_symbols(char *filename);

#endif /* !FTK_SOLIB_H */
